#include "../menu_capture_owner.h"
#include "../pause_menu_capture.h"
#include <cstdio>
#include <cstdint>

static int g_fails = 0;

static void Expect(bool cond, const char* name) {
    if (cond) {
        std::printf("PASS %s\n", name);
        return;
    }
    std::printf("FAIL %s\n", name);
    ++g_fails;
}

struct Sim {
    menucapture::Owner hold;
    std::uint32_t generation = 0;
    int accepts = 0;

    std::uint64_t Focus(int kind) {
        const std::uint64_t token = ((std::uint64_t)++generation << 32) | (std::uint32_t)kind;
        hold.Invalidate(token);
        return token;
    }

    pausecapture::Decision Offer(int kind, std::uint64_t token, std::uintptr_t surface,
                                 int width, int height, bool sceneClassified, bool identified,
                                 bool backbuffer = false, bool colour = true) {
        pausecapture::Offer offer;
        offer.menuClass = kind;
        offer.backbuffer = backbuffer;
        offer.colour = colour;
        offer.sceneClassified = sceneClassified;
        offer.identifiedPauseMovie = identified;
        offer.width = width;
        offer.height = height;
        offer.surface = surface;
        offer.token = token;
        const auto decision = pausecapture::Decide(offer, hold);
        if (decision == pausecapture::Decision::Accept) {
            hold.Capture(token, surface);
            ++accepts;
        }
        return decision;
    }
};

int main() {
    Sim sim;
    const std::uintptr_t mainSurface = 0x1000;
    const std::uintptr_t sceneColour = 0x339BD320;
    const std::uintptr_t sbsBackbuffer = 0xBB;
    const std::uintptr_t unrelated = 0x1024;
    const std::uintptr_t pauseMovie = 0x2000;
    const std::uintptr_t secondPause = 0x4000;

    const auto mainToken = sim.Focus(2);
    Expect(sim.Offer(2, mainToken, mainSurface, 1280, 720, false, true) == pausecapture::Decision::Ignore,
           "main menu is not decided by the pause policy");
    sim.hold.Capture(mainToken, mainSurface);
    Expect(sim.hold.CanProvide(mainToken), "main provider owns its own texture");

    const auto gameplay = sim.Focus(0);
    Expect(!sim.hold.CanProvide(mainToken), "leaving the main menu drops its token");
    Expect(sim.hold.source == 0 && !sim.hold.valid, "gameplay clears the captured source");
    Expect(sim.Offer(3, gameplay, pauseMovie, 1280, 720, false, true) == pausecapture::Decision::Reject,
           "a pause movie is refused while the token is gameplay");

    const auto pause = sim.Focus(3);
    Expect(sim.hold.state == pause && !sim.hold.valid, "pause invalidates before a replacement exists");
    Expect(sim.Offer(3, pause, sceneColour, 5264, 2684, true, false) == pausecapture::Decision::Reject,
           "the 5264x2684 scene colour target is not a pause movie");
    Expect(sim.Offer(3, pause, sceneColour, 5264, 2684, true, true) == pausecapture::Decision::Reject,
           "marking a scene target identified still rejects it");
    Expect(sim.Offer(3, pause, sbsBackbuffer, 5264, 2684, true, false, true) == pausecapture::Decision::Reject,
           "the SBS backbuffer is not a pause movie");
    Expect(sim.Offer(3, pause, unrelated, 1024, 1024, false, false) == pausecapture::Decision::Reject,
           "an unrelated colour target is not a pause movie");
    Expect(sim.Offer(3, pause, unrelated, 1280, 720, false, false) == pausecapture::Decision::Reject,
           "1280x720 without pause-movie identity is not registered");
    Expect(!sim.hold.valid && sim.hold.source == 0, "rejected scene and colour targets create no provider");

    Expect(sim.Offer(3, pause, pauseMovie, 1280, 720, false, true) == pausecapture::Decision::Accept,
           "an identified non-scene pause texture registers");
    Expect(sim.hold.CanProvide(pause) && sim.hold.source == pauseMovie, "pause provides only that texture");
    Expect(!sim.hold.CanProvide(mainToken), "the old main token cannot provide the pause texture");
    Expect(sim.Offer(3, pause, sceneColour, 5264, 2684, true, false) == pausecapture::Decision::Keep,
           "a later scene target does not replace the identified pause texture");
    Expect(sim.hold.source == pauseMovie, "pause identity stays on the identified texture");

    const auto resume = sim.Focus(0);
    Expect(!sim.hold.CanProvide(pause), "resume invalidates the pause token");
    Expect(sim.hold.source == 0 && !sim.hold.valid, "resume does not keep the pause texture");
    Expect(sim.Offer(3, resume, pauseMovie, 1280, 720, false, true) == pausecapture::Decision::Reject,
           "resume refuses the previous pause texture");

    const auto pauseAgain = sim.Focus(3);
    Expect(sim.Offer(3, pauseAgain, sceneColour, 5264, 2684, true, false) == pausecapture::Decision::Reject,
           "the second pause still rejects the scene target");
    Expect(sim.Offer(3, pauseAgain, secondPause, 1280, 720, false, true) == pausecapture::Decision::Accept,
           "the second pause captures its own identified texture");
    Expect(sim.hold.source == secondPause, "the second pause does not revive the first texture");
    Expect(sim.hold.CanProvide(pauseAgain) && !sim.hold.CanProvide(pause),
           "only the current pause token can provide");
    Expect(sim.accepts == 2, "each identified pause generation accepts one texture");

    menucapture::Owner stale;
    stale.Capture(mainToken, mainSurface);
    pausecapture::Offer reused;
    reused.menuClass = 3;
    reused.colour = true;
    reused.identifiedPauseMovie = true;
    reused.width = 1280;
    reused.height = 720;
    reused.surface = mainSurface;
    reused.token = pauseAgain;
    Expect(pausecapture::Decide(reused, stale) == pausecapture::Decision::Reject,
           "a main-menu hold is not reused for a pause token without invalidation");
    Expect(stale.CanProvide(mainToken) && stale.source == mainSurface,
           "the rejected offer leaves the stale hold unchanged");

    return g_fails == 0 ? 0 : 1;
}
