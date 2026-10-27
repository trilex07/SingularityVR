# Singularity VR — wejście do rozgrywki (Steam/xrsim)

Krótka procedura dla agenta wykonującego testy wizualne lub tekstur.

## Procedura

1. Uruchom grę przez runner Steam/xrsim w trybie okienkowym.
2. Poczekaj na gotowość runtime (`FOCUSED`, `projectionViews=2`) i znajdź okno `Singularity`.
3. Ustaw okno gry na pierwszym planie. Nie wysyłaj klawiszy, gdy fokus jest na Steamie albo terminalu.
4. Wyślij `Enter` do aktywnego okna. To przechodzi ekran początkowy do menu tytułowego z `Start Game`.
5. Wyślij ponownie `Enter`. Jeżeli ekran nadal pokazuje tytuł, powtórz `Enter` kilka razy z krótką przerwą.
6. Gdy pojawi się `MAIN MENU` z zaznaczonym `Continue Game`, wysyłaj `Enter` aż menu zniknie.
7. Uznaj wejście za zakończone dopiero wtedy, gdy zrzut pokazuje poziom, HUD oraz broń/ręce — sam napis menu nie jest dowodem gameplayu.

## Fallback menu VR

W jednej zweryfikowanej sesji drugi klawiaturowy `Enter` zatrzymał się na ekranie tytułowym, ale przycisk `A` przez most xrsim otworzył `MAIN MENU`; następnie wielokrotny `Enter` wszedł do gry. Używaj tego fallbacku tylko wtedy, gdy Enter nie zmienia obrazu.

## Dowód poprawnej sesji

- Zweryfikowana sesja: `deployment/agent-test-bridge/weapon-texture-apply-a-route-2/`
- Zrzut gameplayu: `after-enter-repeat_sbs.png`
- W tej sesji log potwierdził `gameplay hook gate: LIVE`, `CINE OFF`, widoczny HUD i broń w obu oczach.

## Ważne

- Logi i liczniki są pomocnicze; zawsze sprawdzaj zrzut obrazu.
- Po teście runner musi przywrócić pliki Steam do baseline. Sprawdź `texture_override.ini`: `mode_apply=1`, `capture=0`.

## Potwierdzona sekwencja Steam — 2026-10-10

Na świeżej instancji uruchomionej przez Steam zadziałała sekwencja samych
klawiszy `Enter` wysyłanych do aktywnego okna `Singularity`:

1. `Enter` ×1 — pierwszy ekran Raven.
2. `Enter` ×2 — podświetlenie/animacja logo Raven.
3. `Enter` ×3 — ekran przejściowy z fragmentami obrazu.
4. `Enter` ×4 — Activision.
5. `Enter` ×5 — Unreal Technology.
6. `Enter` ×6 — przejście/cinematic.
7. `Enter` ×7 — ekran tytułowy Singularity.
8. `Enter` ×8 — właściwy gameplay: dwa obrazy stereo, HUD i broń.

Między naciśnięciami wystarczało około 0,7–1,2 s. W tej sesji nie używano
replayu xrsim ani przycisków `A/B`; kluczowe było utrzymanie fokusu na oknie
gry. Log potwierdził `CINE OFF`, `MenuState=0`, `GloveCensus` oraz
`IndependentHandsAudit ... success=270`.
