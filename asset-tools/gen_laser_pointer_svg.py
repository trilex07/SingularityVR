"""
Generates clean, scalable SVG vector sources for Laser Pointer kit.
"""

import os

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SVG_DIR = os.path.join(BASE_DIR, "assets", "source", "ui_remaster", "laser_pointer")
os.makedirs(SVG_DIR, exist_ok=True)

laser_svg = """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 256" width="512" height="256">
  <defs>
    <filter id="cyanGlow" x="-20%" y="-20%" width="140%" height="140%">
      <feGaussianBlur stdDeviation="6" result="blur"/>
      <feMerge>
        <feMergeNode in="blur"/>
        <feMergeNode in="SourceGraphic"/>
      </feMerge>
    </filter>
  </defs>

  <!-- 1. Dot Idle -->
  <g transform="translate(64, 64)">
    <circle cx="0" cy="0" r="28" fill="#2CE8E4" fill-opacity="0.3" filter="url(#cyanGlow)"/>
    <circle cx="0" cy="0" r="10" fill="#FFFFFF" stroke="#2CE8E4" stroke-width="2"/>
  </g>

  <!-- 2. Dot Active -->
  <g transform="translate(192, 64)">
    <circle cx="0" cy="0" r="40" fill="#78FFFA" fill-opacity="0.5" filter="url(#cyanGlow)"/>
    <circle cx="0" cy="0" r="14" fill="#FFFFFF" stroke="#78FFFA" stroke-width="3"/>
  </g>

  <!-- 3. Hover Ring -->
  <g transform="translate(320, 64)" stroke="#2CE8E4" fill="none" stroke-width="4">
    <path d="M 12,-44 A 46,46 0 0,1 44,-12"/>
    <path d="M 44,12 A 46,46 0 0,1 12,44"/>
    <path d="M -12,44 A 46,46 0 0,1 -44,12"/>
    <path d="M -44,-12 A 46,46 0 0,1 -12,-44"/>
    <line x1="-54" y1="0" x2="-38" y2="0" stroke="#78FFFA" stroke-width="2"/>
    <line x1="38" y1="0" x2="54" y2="0" stroke="#78FFFA" stroke-width="2"/>
    <line x1="0" y1="-54" x2="0" y2="-38" stroke="#78FFFA" stroke-width="2"/>
    <line x1="0" y1="38" x2="0" y2="54" stroke="#78FFFA" stroke-width="2"/>
  </g>

  <!-- 4. Click Burst -->
  <g transform="translate(448, 64)" stroke="#78FFFA" stroke-width="2">
    <circle cx="0" cy="0" r="48" fill="none" stroke="#2CE8E4" stroke-width="3" stroke-dasharray="8,6"/>
    <line x1="0" y1="-20" x2="0" y2="-52"/>
    <line x1="0" y1="20" x2="0" y2="52"/>
    <line x1="-20" y1="0" x2="-52" y2="0"/>
    <line x1="20" y1="0" x2="52" y2="0"/>
    <line x1="-15" y1="-15" x2="-38" y2="-38"/>
    <line x1="15" y1="-15" x2="38" y2="-38"/>
    <line x1="-15" y1="15" x2="-38" y2="38"/>
    <line x1="15" y1="15" x2="38" y2="38"/>
    <circle cx="0" cy="0" r="8" fill="#FFFFFF"/>
  </g>
</svg>
"""

with open(os.path.join(SVG_DIR, "laser_pointer_kit.svg"), "w", encoding="utf-8") as f:
    f.write(laser_svg)

print("Generated laser_pointer_kit.svg")
