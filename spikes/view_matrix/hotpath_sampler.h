#pragma once

#include <windows.h>
#include <cstdint>

// Fixed-storage, sampled CPU accounting for short diagnostic windows. The
// disabled path is one atomic flag read; no timer, allocation, lock, or format
// work occurs from a hook. Counts are exact. Timings are sampled every 32nd
// call and scaled by count/sample count when summarized.
namespace hotpath {

enum class Family : unsigned {
	native_input_poll,
	native_gamepad_enumeration,
	native_gamepad_callback_replay,
	draw_hook,
	menu_classification,
	foreground_classification,
	shadow_classification,
	track_fp_caster,
	ik_arm_solve,
	bone_palette,
	set_texture,
	other_set_hooks,
	xr_wait_frame,
	pose_input_update,
	d3d9_to_d3d11_copy,
	xr_image_work,
	xr_submit,
	frame_total,
	native_draw_forward,
	count
};

enum class Counter : unsigned {
	xr_sync_actions,
	xr_action_state,
	xr_locate_views,
	xr_locate_left,
	xr_locate_right,
	track_fp_candidate,
	track_fp_get_stream_source,
	track_fp_get_indices,
	track_fp_vb_ref_acquired,
	track_fp_ib_ref_acquired,
	track_fp_matched_caster,
	track_fp_vb_release,
	track_fp_ib_release,
	count
};

inline constexpr unsigned sample_stride = 32;
inline constexpr unsigned native_draw_stride = 512;
inline constexpr unsigned default_window_frames = 1200;
// Standalone overlay calls must not be charged against profiled draw hooks.
inline thread_local unsigned draw_scope_depth = 0;

struct record {
	volatile LONG64 calls{};
	volatile LONG64 samples{};
	alignas(8) volatile LONG64 ticks{};
	volatile LONG sequence{};
};

struct result {
	std::uint64_t calls{};
	std::uint64_t samples{};
	std::uint64_t ticks{};
};

class sampler {
	volatile LONG active_{};
	volatile LONG armed_{};
	volatile LONG frames_left_{};
	volatile LONG delay_left_{};
	unsigned window_frames_{default_window_frames};
	unsigned delay_frames_{};
	alignas(8) volatile LONG64 frame_token_{};
	alignas(8) volatile LONG64 counters_[static_cast<unsigned>(Counter::count)]{};
	record records_[static_cast<unsigned>(Family::count)]{};
	LARGE_INTEGER frequency_{};

public:
	void configure(bool enabled, unsigned frames = default_window_frames, unsigned delay_frames = 0) noexcept {
		InterlockedExchange(&active_, 0);
		InterlockedExchange(&armed_, enabled ? 1 : 0);
		for (auto& r : records_) {
			InterlockedExchange64(&r.calls, 0);
			InterlockedExchange64(&r.samples, 0);
			InterlockedExchange64(&r.ticks, 0);
			InterlockedExchange(&r.sequence, 0);
		}
		for (auto& c : counters_) InterlockedExchange64(&c, 0);
		InterlockedExchange64(&frame_token_, 0);
		QueryPerformanceFrequency(&frequency_);
		window_frames_ = frames ? frames : 1;
		delay_frames_ = delay_frames;
		InterlockedExchange(&frames_left_, static_cast<LONG>(window_frames_));
		InterlockedExchange(&delay_left_, enabled ? static_cast<LONG>(delay_frames) : 0);
		InterlockedExchange(&active_, enabled && delay_frames == 0 ? 1 : 0);
	}

	bool active() const noexcept { return InterlockedCompareExchange(const_cast<volatile LONG*>(&active_), 0, 0) != 0; }
	// Stop a bounded diagnostic at a frame boundary without resetting records or
	// invalidating scopes already in flight. Such scopes may finish normally.
	void stop() noexcept {
		InterlockedExchange(&active_,0);
		InterlockedExchange(&armed_,0);
	}
	std::uint64_t frame_token() const noexcept {
		return static_cast<std::uint64_t>(InterlockedCompareExchange64(
			const_cast<volatile LONG64*>(&frame_token_), 0, 0));
	}
	std::uint64_t next_xr_frame() noexcept {
		if (!active()) return frame_token();
		return static_cast<std::uint64_t>(InterlockedIncrement64(&frame_token_));
	}

	// Called once from Present. Returns true exactly when the bounded window ends.
	bool advance_frame() noexcept {
		if (!active()) {
			if (!InterlockedCompareExchange(&armed_, 0, 0)) return false;
			if (InterlockedDecrement(&delay_left_) > 0) return false;
			for (auto& r : records_) {
				InterlockedExchange64(&r.calls, 0);
				InterlockedExchange64(&r.samples, 0);
				InterlockedExchange64(&r.ticks, 0);
				InterlockedExchange(&r.sequence, 0);
			}
			for (auto& c : counters_) InterlockedExchange64(&c, 0);
			InterlockedExchange64(&frame_token_, 0);
			InterlockedExchange(&frames_left_, static_cast<LONG>(window_frames_));
			InterlockedExchange(&active_, 1);
			return false;
		}
		if (InterlockedDecrement(&frames_left_) > 0) return false;
		InterlockedExchange(&active_, 0);
		InterlockedExchange(&armed_, 0);
		return true;
	}

	void count(Counter c) noexcept {
		if (!active()) return;
		InterlockedIncrement64(&counters_[static_cast<unsigned>(c)]);
	}
	std::uint64_t counter(Counter c) const noexcept {
		return static_cast<std::uint64_t>(InterlockedCompareExchange64(
			const_cast<volatile LONG64*>(&counters_[static_cast<unsigned>(c)]), 0, 0));
	}

	void record_call(Family f) noexcept {
		InterlockedIncrement64(&records_[static_cast<unsigned>(f)].calls);
	}
	bool should_time(Family f) noexcept {
		const LONG sequence = InterlockedIncrement(&records_[static_cast<unsigned>(f)].sequence);
		// Enumeration was measured at ~75ms: time every bounded-window scan,
		// rather than estimating its whole cost from nine samples.
		const unsigned stride=f==Family::native_draw_forward?native_draw_stride:sample_stride;
		return active() && (f==Family::native_gamepad_enumeration || f==Family::native_gamepad_callback_replay || sequence % static_cast<LONG>(stride) == 0);
	}
	void record_sample(Family f, LONGLONG elapsed) noexcept {
		auto& r = records_[static_cast<unsigned>(f)];
		InterlockedExchangeAdd64(&r.ticks, elapsed);
		InterlockedIncrement64(&r.samples);
	}
	result read(Family f) const noexcept {
		const auto& r = records_[static_cast<unsigned>(f)];
		return {
			static_cast<std::uint64_t>(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.calls), 0, 0)),
			static_cast<std::uint64_t>(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.samples), 0, 0)),
			static_cast<std::uint64_t>(InterlockedCompareExchange64(const_cast<volatile LONG64*>(&r.ticks), 0, 0))
		};
	}
	double milliseconds_per_frame(Family f, unsigned frames) const noexcept {
		const auto r = read(f);
		if (!r.samples || !frequency_.QuadPart || !frames) return 0.0;
		return (double(r.ticks) / double(r.samples)) * (double(r.calls) / double(frames)) *
			1000.0 / double(frequency_.QuadPart);
	}
	double microseconds_per_call(Family f) const noexcept {
		const auto r = read(f);
		if (!r.samples || !frequency_.QuadPart) return 0.0;
		return (double(r.ticks) / double(r.samples)) * 1000000.0 / double(frequency_.QuadPart);
	}
	unsigned configured_frames() const noexcept { return window_frames_; }
	unsigned completed_frames() const noexcept {
		if(!frequency_.QuadPart)return 0;
		const LONG left=InterlockedCompareExchange(const_cast<volatile LONG*>(&frames_left_),0,0);
		return left>=0 && static_cast<unsigned>(left)<=window_frames_ ? window_frames_-static_cast<unsigned>(left) : 0;
	}
	unsigned configured_delay_frames() const noexcept { return delay_frames_; }
};

class scope {
	sampler* owner_{};
	Family family_{};
	LARGE_INTEGER started_{};
	bool timed_{};
	bool finished_{};
	bool draw_entered_{};

public:
	scope(sampler& owner, Family family) noexcept : family_(family) {
		if (!owner.active()) return;
		owner_ = &owner;
		if(family_==Family::draw_hook){++draw_scope_depth;draw_entered_=true;}
		owner_->record_call(family_);
		if (owner_->should_time(family_)) {
			QueryPerformanceCounter(&started_);
			timed_ = true;
		}
	}
	~scope() {
		finish();
	}
	void finish() noexcept {
		if (finished_) return;
		finished_ = true;
		if(draw_entered_){--draw_scope_depth;draw_entered_=false;}
		if (!timed_ || !owner_) return;
		LARGE_INTEGER ended{};
		QueryPerformanceCounter(&ended);
		owner_->record_sample(family_, ended.QuadPart - started_.QuadPart);
		timed_ = false;
	}
	scope(const scope&) = delete;
	scope& operator=(const scope&) = delete;
};

// Observe only an existing native-call boundary. No new hook/state query,
// changed arguments, replay or allocations. Forward directly when disabled.
template<class Forward> auto forward_draw(sampler& owner,Forward&& forward) -> decltype(forward()) {
	if(!draw_scope_depth)return forward();
	scope native(owner,Family::native_draw_forward);
	return forward();
}

inline const char* name(Family f) noexcept {
	switch (f) {
	case Family::native_input_poll: return "native_input_poll";
	case Family::native_gamepad_enumeration: return "native_gamepad_enum";
	case Family::native_gamepad_callback_replay: return "native_gamepad_replay";
	case Family::draw_hook: return "draw";
	case Family::menu_classification: return "menu";
	case Family::foreground_classification: return "foreground";
	case Family::shadow_classification: return "shadow";
	case Family::track_fp_caster: return "TrackFpCaster";
	case Family::ik_arm_solve: return "ik";
	case Family::bone_palette: return "palette";
	case Family::set_texture: return "set_texture";
	case Family::other_set_hooks: return "other_set";
	case Family::xr_wait_frame: return "wait_frame";
	case Family::pose_input_update: return "pose_input";
	case Family::d3d9_to_d3d11_copy: return "d3d_copy";
	case Family::xr_image_work: return "xr_images";
	case Family::xr_submit: return "xr_submit";
	case Family::frame_total: return "present_frame";
	case Family::native_draw_forward: return "native_draw_forward";
	default: return "unknown";
	}
}

} // namespace hotpath
