# SIDE — ekran startowy SINGULARITY VR

Jesteś SIDE. Nie ruszasz Steam, DLL, INI, gałęzi MAIN ani worktree
`SingularityVR-post-opening`. Własny branch i worktree, commit i push, bez merge.

## Co widać

Przy uruchomieniu gry, zanim dojdzie do menu, jest czarna plansza z napisem
`SINGULARITY`. Białe, szerokie, geometryczne litery, miękka poświata, duży
margines czerni, bez ramki i bez drugiego wiersza. To nie jest pole tekstowe
menu, które MAIN już poprawił (`iis` znak 10 i `SPMenus` znak 166). Tamten napis
zostaw.

Zrzut użytkownika pokazuje sam ten wordmark. Ma powstać ten sam znak z dopiskiem
`VR`, w tej samej wadze, szerokości i poświacie. Nie podmieniaj kroju na
generyczny sans i nie rozjeżdżaj litery `SINGULARITY`.

## Najpierw dowód

Ustal, który plik naprawdę rysuje tę planszę. Kandydaci do sprawdzenia, nie do
zgadywania:

- film Bink w katalogu gry,
- tekstura `ui_logos.logo` z `ui_logos_SF.xxx` (512×128, DXT5, już zdekodowana
  offline jako oryginalne logo),
- inny movie odtwarzany przed `RvUIGFxMovie_IIS`.

Dopiero po identyfikacji producenta przygotuj podmianę. Jeśli to tekstura,
zostaw wymiary, format i mipmapy. Jeśli to film, zachowaj czas, proporcje i
ciszę albo oryginalny dźwięk. Nie publikuj wyciągniętych assetów gry.

## Granice

Nie zmieniaj rąk, chwytu, kina, cieni, kursora, HUD-u, dubbingu ani stereo.
Nie wgrywaj nic do Steam. Wynik to asset, skrypt odtwarzalny, hash wejścia i
wyjścia oraz krótki handoff dla MAIN. Fizyczne potwierdzenie należy do MAIN.
