"""Rewrite the two mockups that picked up a bad byte. ASCII source, UTF-8 output."""
from pathlib import Path

OUT = Path(__file__).resolve().parents[2] / "docs" / "hud" / "mockups"

CALM = """<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="720" viewBox="0 0 1280 720">
  <rect width="1280" height="720" fill="#121416"/>
  <rect x="64" y="36" width="1152" height="648" fill="#181b1e" stroke="#2c3136"/>
  <circle cx="640" cy="360" r="300" fill="none" stroke="#5a6f8d" stroke-dasharray="5 7"/>
  <text x="80" y="28" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">Spokojna rozgrywka. Ko\u0142o jest polem gogli, prostok\u0105t klatk\u0105 16:9.</text>
  <g opacity="0.28">
    <text x="88" y="640" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="22">100</text>
    <text x="1088" y="640" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="22">12 / 48</text>
    <text x="88" y="662" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="12">oryginalny naro\u017cnik, poza ko\u0142em</text>
  </g>
  <rect x="500" y="250" width="280" height="160" fill="none" stroke="#2c3136" stroke-dasharray="3 6"/>
  <text x="640" y="336" text-anchor="middle" fill="#5c656d" font-family="Segoe UI, Arial, sans-serif" font-size="14">\u015brodek wolny</text>
  <g>
    <rect x="230" y="470" width="250" height="78" fill="#101214" stroke="#5a6f8d"/>
    <text x="246" y="492" fill="#5a6f8d" font-family="Segoe UI, Arial, sans-serif" font-size="13">NADGARSTEK - ZDROWIE</text>
    <g fill="#e6e1d6">
      <rect x="246" y="508" width="16" height="22"/>
      <rect x="266" y="508" width="16" height="22"/>
      <rect x="286" y="508" width="16" height="22"/>
      <rect x="306" y="508" width="16" height="22"/>
      <rect x="326" y="508" width="16" height="22"/>
      <rect x="346" y="508" width="16" height="22"/>
      <rect x="366" y="508" width="16" height="22"/>
      <rect x="386" y="508" width="16" height="22"/>
      <rect x="406" y="508" width="16" height="22"/>
      <rect x="426" y="508" width="16" height="22"/>
    </g>
    <text x="452" y="526" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="16">10/10</text>
  </g>
  <g>
    <rect x="800" y="470" width="250" height="78" fill="#101214" stroke="#5a6f8d"/>
    <text x="816" y="492" fill="#5a6f8d" font-family="Segoe UI, Arial, sans-serif" font-size="13">BRO\u0143 - NABOJE</text>
    <text x="816" y="530" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="32">12</text>
    <text x="870" y="530" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="18">/ 48</text>
    <rect x="980" y="508" width="48" height="16" fill="none" stroke="#e6e1d6"/>
    <rect x="986" y="512" width="36" height="8" fill="#e6e1d6"/>
  </g>
</svg>
"""

FIGHT = """<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="720" viewBox="0 0 1280 720">
  <rect width="1280" height="720" fill="#121416"/>
  <rect x="64" y="36" width="1152" height="648" fill="#181b1e" stroke="#2c3136"/>
  <circle cx="640" cy="360" r="300" fill="none" stroke="#5a6f8d" stroke-dasharray="5 7"/>
  <text x="80" y="28" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">Walka. Niskie zdrowie i pusty magazynek. \u015arodek zostaje wolny.</text>
  <rect x="500" y="220" width="280" height="180" fill="none" stroke="#2c3136" stroke-dasharray="3 6"/>
  <text x="640" y="316" text-anchor="middle" fill="#5c656d" font-family="Segoe UI, Arial, sans-serif" font-size="14">wr\u00f3g, bez celownika</text>
  <rect x="348" y="348" width="28" height="8" fill="#c4a46a"/>
  <text x="348" y="340" fill="#c4a46a" font-family="Segoe UI, Arial, sans-serif" font-size="12">Z LEWEJ</text>
  <g>
    <rect x="230" y="470" width="270" height="86" fill="#101214" stroke="#c4a46a"/>
    <text x="246" y="492" fill="#c4a46a" font-family="Segoe UI, Arial, sans-serif" font-size="13">NADGARSTEK - MA\u0141O</text>
    <g>
      <rect x="246" y="508" width="16" height="22" fill="#e6e1d6"/>
      <rect x="266" y="508" width="16" height="22" fill="#e6e1d6"/>
      <rect x="286" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
      <rect x="306" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
      <rect x="326" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
      <rect x="346" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
      <rect x="366" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
      <rect x="386" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
      <rect x="406" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
      <rect x="426" y="508" width="16" height="22" fill="none" stroke="#5a6f8d"/>
    </g>
    <text x="452" y="526" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="16">2/10</text>
  </g>
  <g>
    <rect x="790" y="450" width="280" height="110" fill="#101214" stroke="#c4a46a"/>
    <text x="806" y="472" fill="#c4a46a" font-family="Segoe UI, Arial, sans-serif" font-size="13">BRO\u0143 - PUSTY MAGAZYNEK</text>
    <text x="806" y="516" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="36">0</text>
    <text x="848" y="516" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="18">/ 20</text>
    <rect x="940" y="492" width="52" height="18" fill="none" stroke="#e6e1d6"/>
    <text x="806" y="544" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="16">PRZE\u0141ADUJ</text>
  </g>
</svg>
"""


SWITCH = """<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="720" viewBox="0 0 1280 720">
  <rect width="1280" height="720" fill="#121416"/>
  <rect x="64" y="36" width="1152" height="648" fill="#181b1e" stroke="#2c3136"/>
  <circle cx="640" cy="360" r="300" fill="none" stroke="#5a6f8d" stroke-dasharray="5 7"/>
  <text x="80" y="28" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">Zmiana broni. Nazwa ga\u015bnie po oko\u0142o 1,2 s. CommonText.ARPistol.</text>
  <rect x="500" y="250" width="280" height="140" fill="none" stroke="#2c3136" stroke-dasharray="3 6"/>
  <text x="640" y="326" text-anchor="middle" fill="#5c656d" font-family="Segoe UI, Arial, sans-serif" font-size="14">\u015brodek wolny</text>
  <g>
    <rect x="230" y="490" width="230" height="64" fill="#101214" stroke="#5a6f8d"/>
    <text x="246" y="514" fill="#5a6f8d" font-family="Segoe UI, Arial, sans-serif" font-size="13">ZDROWIE</text>
    <text x="246" y="538" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="18">10/10</text>
  </g>
  <g>
    <rect x="760" y="430" width="300" height="130" fill="#101214" stroke="#e6e1d6"/>
    <text x="776" y="456" fill="#5a6f8d" font-family="Segoe UI, Arial, sans-serif" font-size="13">TERAZ</text>
    <text x="776" y="496" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="28">Centurion</text>
    <text x="776" y="534" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="22">12</text>
    <text x="820" y="534" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">/ 48</text>
  </g>
</svg>
"""

INTERACT = """<svg xmlns="http://www.w3.org/2000/svg" width="1280" height="720" viewBox="0 0 1280 720">
  <rect width="1280" height="720" fill="#121416"/>
  <rect x="64" y="36" width="1152" height="648" fill="#181b1e" stroke="#2c3136"/>
  <circle cx="640" cy="360" r="300" fill="none" stroke="#5a6f8d" stroke-dasharray="5 7"/>
  <text x="80" y="28" fill="#8d97a1" font-family="Segoe UI, Arial, sans-serif" font-size="16">Interakcja. Prompt pod \u015brodkiem, w dotychczasowym pasie, wewn\u0105trz ko\u0142a.</text>
  <rect x="560" y="230" width="160" height="90" fill="none" stroke="#3c4248"/>
  <text x="640" y="282" text-anchor="middle" fill="#5c656d" font-family="Segoe UI, Arial, sans-serif" font-size="14">notatka w \u015bwiecie</text>
  <g>
    <rect x="470" y="390" width="340" height="56" fill="#101214" stroke="#5a6f8d"/>
    <rect x="488" y="406" width="18" height="24" fill="none" stroke="#e6e1d6"/>
    <rect x="494" y="412" width="6" height="12" fill="#e6e1d6"/>
    <text x="522" y="426" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="22">Notatka</text>
  </g>
  <g>
    <rect x="230" y="520" width="180" height="48" fill="#101214" stroke="#5a6f8d"/>
    <text x="246" y="550" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="16">10/10</text>
  </g>
  <g>
    <rect x="860" y="520" width="180" height="48" fill="#101214" stroke="#5a6f8d"/>
    <text x="876" y="550" fill="#e6e1d6" font-family="Segoe UI, Arial, sans-serif" font-size="16">12 / 48</text>
  </g>
</svg>
"""


def main() -> None:
    (OUT / "01-spokoj.svg").write_text(CALM, encoding="utf-8")
    (OUT / "02-walka.svg").write_text(FIGHT, encoding="utf-8")
    (OUT / "03-zmiana-broni.svg").write_text(SWITCH, encoding="utf-8")
    (OUT / "04-interakcja.svg").write_text(INTERACT, encoding="utf-8")


if __name__ == "__main__":
    main()
