# Polityka: „podobny głos” vs klon osoby (NC mod)

## Wiesława Szymborska i inni rozpoznawalni artyści

**Nie szukamy, nie pobieramy i nie używamy nagrań głosu Wisławy Szymborskiej** (ani innych zmarłych lub żyjących osób publicznych) do syntezy, klonowania ani „głosu w jej stylu” w modzie.

Powody:

- Nagrania odczytów wierszy to **utwory / wykonania** objęte prawem autorskim i prawami pokrewnymi (fonogram, wykonawca, często wytwórnia lub archiwum).
- **Śmierć autorki nie przenosi automatycznie** prawa do syntezy głosu na modderów.
- Pocket-TTS / podobne narzędzia wymagają **wyraźnej, lawful zgody** na klonowanie; publiczne posłuchanie w internecie **nie jest** taką zgodą.
- Mod jest darmowy, ale nadal **publikacją** — imitacja rozpoznawalnego głosu to ryzyko prawne i wizerunkowe.

Szukając „próbek Szymborskiej do AI” **celowo nie dostarczamy linków** — to byłoby przygotowanie do niedozwolonego klonu, a nie do legalnego dubbingu.

## Co możemy zrobić zamiast tego (legalnie)

| Cel br br br brzmienia | Legalna ścieżka |
|---|---|
| Spokojny, literacki narrator | Twoje modulacje + ewentualnie **Natan/Magda** (RHVoice), **CML** (anonim), **Twoje intro** jako benchmark jakości |
| Stanowczy lektor UI | Twoja modulacja `mod_stanowczy` / Natan / mc_speech |
| „Mądra, dojrzała” kobieta w grze | **Magda** (CC BY), Piper Gosia — **fikcyjna postać**, nie imitacja poetki |
| Męski głos archiwalny / dokument | Natan, mc_speech, Twoje nagrania — **bez odwołania do konkretnej osoby** |

Możemy napisać **krótki brief reżyserski** (tempo, miękkość konsonantów, brak pathosu) inspirowany *typem* lektora poezji, **bez** odwołania do konkretnej osoby i **bez** jej nagrań.

## Osoby / głosy, które **wolno** używać w tym projekcie (NC)

Zobacz `docs/POLISH_VOICE_REFERENCES_LICENSED.md` i `docs/POLISH_CAST_LEGAL_20261014.md`:

- **Ty** — własne próbki modulacji (`incoming_modulations/`).  
- **RHVoice Natan** (CC0), **Magda** (CC BY + atrybucja).  
- **Piper** pl_PL (mc_speech, gosia; darkman przy NC).  
- **CML-TTS / LibriVox** — tylko **anonimowo**, bez identyfikacji mówcy.  
- **Pocket-TTS** + Twoje lub powyższe referencje (CC BY model).

## Następny krok

1. Wrzuć modulacje do `audio/polish/user_reference/incoming_modulations/`.  
2. Napisz w czacie, która ma iść na **lektor UI**, która na **fabułę**.  
3. Uruchomimy `prepare_user_modulation_candidates.py` i reel A/B z Natan/Magda/spokoj.
