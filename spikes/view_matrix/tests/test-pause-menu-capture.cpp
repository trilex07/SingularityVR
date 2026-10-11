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
                                 int width, int height, bool backbuffer = false, bool colour = true) {
        pausecapture::Offer offer;
        offer.menuClass = kind;
        offer.backbuffer = backbuffer;
        offer.colour = colour;
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
    const std::uintptr_t pauseSurface = 0x2000;
    const std::uintptr_t worldSurface = 0x3000;
    const std::uintptr_t secondPause = 0x4000;

    const auto mainToken = sim.Focus(2);
    Expect(sim.Offer(2, mainToken, mainSurface, 1280, 720) == pausecapture::Decision::Ignore,
           "main menu is not decided by the pause policy");
    sim.hold.Capture(mainToken, mainSurface);
    Expect(sim.hold.CanProvide(mainToken), "main provider owns its own texture");
    Expect(sim.hold.source == mainSurface, "main surface identity is recorded");

    const auto gameplay = sim.Focus(0);
    Expect(!sim.hold.CanProvide(mainToken), "leaving the main menu drops its token");
    Expect(sim.hold.source == 0 && !sim.hold.valid, "gameplay clears the captured source");
    Expect(sim.Offer(3, gameplay, pauseSurface, 5264, 2684) == pausecapture::Decision::Reject,
           "a pause-sized target is refused while the token is gameplay");
    Expect(!sim.hold.CanProvide(gameplay), "gameplay still provides nothing");

    const auto pause = sim.Focus(3);
    Expect(sim.hold.state == pause && !sim.hold.valid, "pause invalidates before a replacement exists");
    Expect(sim.Offer(3, pause, pauseSurface, 5264, 2684) == pausecapture::Decision::Accept,
           "the scene-sized pause movie registers under the new token");
    Expect(sim.hold.CanProvide(pause), "pause can provide the surface it just captured");
    Expect(sim.hold.source == pauseSurface, "pause source is the new surface");
    Expect(!sim.hold.CanProvide(mainToken), "the old main token cannot provide the pause surface");

    Expect(sim.Offer(3, pause, worldSurface, 5264, 2684) == pausecapture::Decision::Keep,
           "a later scene target does not replace the pause movie");
    Expect(sim.hold.source == pauseSurface, "pause identity stays on the first surface");
    Expect(sim.Offer(2, pause, mainSurface, 1280, 720) == pausecapture::Decision::Ignore,
           "a class-2 offer during pause does not touch the pause hold");
    Expect(sim.hold.source == pauseSurface, "class 2 ignore leaves the pause source in place");

    const auto resume = sim.Focus(0);
    Expect(!sim.hold.CanProvide(pause), "resume invalidates the pause token");
    Expect(sim.hold.source == 0 && !sim.hold.valid, "resume does not keep the pause texture");
    Expect(sim.Offer(3, resume, pauseSurface, 5264, 2684) == pausecapture::Decision::Reject,
           "resume refuses the previous pause surface");

    const auto pauseAgain = sim.Focus(3);
    Expect(sim.Offer(3, pauseAgain, secondPause, 5264, 2684) == pausecapture::Decision::Accept,
           "the second pause captures its own surface");
    Expect(sim.hold.source == secondPause, "the second pause does not revive the first surface");
    Expect(sim.hold.CanProvide(pauseAgain), "the second pause token can provide");
    Expect(!sim.hold.CanProvide(pause), "the first pause token stays invalid");
    Expect(sim.accepts == 2, "each pause generation accepts one surface");

    menucapture::Owner stale;
    stale.Capture(mainToken, mainSurface);
    pausecapture::Offer reused;
    reused.menuClass = 3;
    reused.colour = true;
    reused.width = 5264;
    reused.height = 2684;
    reused.surface = mainSurface;
    reused.token = pauseAgain;
    Expect(pausecapture::Decide(reused, stale) == pausecapture::Decision::Reject,
           "a main-menu hold is not reused for a pause token without invalidation");
    Expect(stale.CanProvide(mainToken) && stale.source == mainSurface,
           "the rejected offer leaves the stale hold unchanged");

    const auto fresh = sim.Focus(3);
    Expect(sim.Offer(3, fresh, 0x5000, 5264, 2684, true) == pausecapture::Decision::Reject,
           "the backbuffer is not a pause movie");
    Expect(sim.Offer(3, fresh, 0x5000, 320, 180) == pausecapture::Decision::Reject,
           "a tiny target is not a pause movie");
    Expect(sim.Offer(3, fresh, 0x5000, 5264, 2684, false, false) == pausecapture::Decision::Reject,
           "a non-colour target is not a pause movie");
    Expect(!sim.hold.valid, "rejected pause offers do not create a provider");
    Expect(sim.Offer(3, fresh, 0x6000, 1280, 720) == pausecapture::Decision::Accept,
           "a private 1280x720 pause target is accepted when it is the first one");
    Expect(sim.hold.source == 0x6000, "the accepted pause surface is the one that was offered");

    return g_fails == 0 ? 0 : 1;
}
