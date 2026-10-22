"""
Generates clean, scalable SVG vector sources for the Weapon Wheel system and weapon icons.
"""

import os

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SVG_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "weapon_wheel")
os.makedirs(SVG_DIR, exist_ok=True)

# 1. Weapon Wheel System SVG
wheel_svg = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024" width="1024" height="1024">
  <defs>
    <radialGradient id="ringBg" cx="50%" cy="50%" r="50%">
      <stop offset="60%" stop-color="#10161E" stop-opacity="0.95"/>
      <stop offset="90%" stop-color="#141D27" stop-opacity="0.98"/>
      <stop offset="100%" stop-color="#2B3A48" stop-opacity="1.0"/>
    </radialGradient>
    <radialGradient id="hubGlow" cx="50%" cy="50%" r="50%">
      <stop offset="0%" stop-color="#78FFFA" stop-opacity="1.0"/>
      <stop offset="40%" stop-color="#2CE8E4" stop-opacity="0.8"/>
      <stop offset="80%" stop-color="#0E464E" stop-opacity="0.3"/>
      <stop offset="100%" stop-color="#081014" stop-opacity="0.0"/>
    </radialGradient>
    <filter id="cyanGlow" x="-20%" y="-20%" width="140%" height="140%">
      <feGaussianBlur stdDeviation="8" result="blur"/>
      <feMerge>
        <feMergeNode in="blur"/>
        <feMergeNode in="SourceGraphic"/>
      </feMerge>
    </filter>
  </defs>

  <!-- Outer Bezel & Gauge Ring -->
  <circle cx="512" cy="512" r="480" fill="url(#ringBg)" stroke="#415566" stroke-width="4"/>
  <circle cx="512" cy="512" r="465" fill="none" stroke="#23303C" stroke-width="2"/>
  <circle cx="512" cy="512" r="440" fill="none" stroke="#2CE8E4" stroke-width="1.5" stroke-dasharray="4,8" opacity="0.6"/>

  <!-- Dial Markings -->
  <g stroke="#7D919E" stroke-width="2" opacity="0.8">
    <line x1="512" y1="36" x2="512" y2="72" stroke="#2CE8E4" stroke-width="4"/>
    <line x1="512" y1="952" x2="512" y2="988" stroke="#2CE8E4" stroke-width="4"/>
    <line x1="36" y1="512" x2="72" y2="512" stroke="#2CE8E4" stroke-width="4"/>
    <line x1="952" y1="512" x2="988" y2="512" stroke="#2CE8E4" stroke-width="4"/>
  </g>

  <!-- Stenciled Technical Markings -->
  <text x="512" y="60" font-family="Bahnschrift, Arial" font-size="14" font-weight="bold" fill="#82A5B9" text-anchor="middle" letter-spacing="4">КАТОРГА-12 // СЕКТОР ВЫБОРА ВООРУЖЕНИЯ</text>
  <text x="512" y="976" font-family="Bahnschrift, Arial" font-size="14" font-weight="bold" fill="#82A5B9" text-anchor="middle" letter-spacing="4">ГОСТ 1955 // ЭЛЕМЕНТ-99 // СИНГУЛЯРНОСТЬ</text>

  <!-- North Sector (Idle) -->
  <path d="M 380,210 A 435,435 0 0,1 644,210 L 575,370 A 185,185 0 0,0 449,370 Z" fill="#121A24" fill-opacity="0.8" stroke="#3C5569" stroke-width="2"/>
  <!-- East Sector (Active/Hovered with Glow) -->
  <path d="M 814,380 A 435,435 0 0,1 814,644 L 654,575 A 185,185 0 0,0 654,449 Z" fill="#183C4E" fill-opacity="0.9" stroke="#2CE8E4" stroke-width="4" filter="url(#cyanGlow)"/>
  <!-- South Sector (Idle) -->
  <path d="M 644,814 A 435,435 0 0,1 380,814 L 449,654 A 185,185 0 0,0 575,654 Z" fill="#121A24" fill-opacity="0.8" stroke="#3C5569" stroke-width="2"/>
  <!-- West Sector (Idle) -->
  <path d="M 210,644 A 435,435 0 0,1 210,380 L 370,449 A 185,185 0 0,0 370,575 Z" fill="#121A24" fill-opacity="0.8" stroke="#3C5569" stroke-width="2"/>

  <!-- Center Hub Containment Vessel -->
  <circle cx="512" cy="512" r="185" fill="#121820" stroke="#4B5F73" stroke-width="4"/>
  <circle cx="512" cy="512" r="150" fill="#0A0E12" stroke="#1A2832" stroke-width="2"/>
  <circle cx="512" cy="512" r="90" fill="url(#hubGlow)"/>
  <circle cx="512" cy="512" r="45" fill="#78FFFA" opacity="0.9" filter="url(#cyanGlow)"/>

  <!-- Aperture mechanical iris lines -->
  <g stroke="#324B5F" stroke-width="2">
    <line x1="512" y1="362" x2="575" y2="445"/>
    <line x1="642" y1="437" x2="575" y2="575"/>
    <line x1="642" y1="587" x2="512" y2="575"/>
    <line x1="512" y1="662" x2="449" y2="575"/>
    <line x1="382" y1="587" x2="449" y2="449"/>
    <line x1="382" y1="437" x2="512" y2="449"/>
  </g>

  <!-- Center Label -->
  <text x="512" y="640" font-family="Bahnschrift, Arial" font-size="15" font-weight="bold" fill="#D4DFE6" text-anchor="middle" letter-spacing="2">TMD-E99</text>
</svg>
"""

with open(os.path.join(SVG_DIR, "weapon_wheel_system.svg"), "w", encoding="utf-8") as f:
    f.write(wheel_svg)

print("Generated weapon_wheel_system.svg")
