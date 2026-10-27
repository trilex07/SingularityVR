$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$source = Get-Content -Raw -LiteralPath (Join-Path $root 'd3d9.cpp')
$overlay = Get-Content -Raw -LiteralPath (Join-Path $root 'vr_menu_overlay.h')
if ($source -match 'Log\("independent hands:') { throw 'per-frame independent-hand disk logging returned' }
if ($source -notmatch 'inline bool ArmsAnimActive\(\) \{ return Rd\(g_armsAnimActiveCached\)') { throw 'draw hooks still evaluate 64-bit animation deadlines' }
if ($source -notmatch 'Hook_SetTexture' -or $source -notmatch 'IsProjectedShadowMap') { throw 'sampled shadow projection path is absent' }
if ($source -match 'if \(Rd\(g_vrUiMenu\)\) return StereoPair') { throw 'menu still bypasses the canonical HUD path' }
if ($overlay -notmatch 'VrUiProjectEye\(eye,\s*g_vrUiHit\.world') { throw 'hit marker does not use the OpenXR eye projector' }
$ini = Get-Content -Raw -LiteralPath (Join-Path $root 'SingularityVR.ini')
if ($ini -match '(?m)^RenderEvidence=1$' -or $ini -match '(?m)^CameraTranslationEvidence=1$') { throw 'expensive evidence mode is enabled in shipping ini' }
# Regression for 2026-10-09: the shipping ini carried GloveIsolate=0 while the
# comment directly above it mandated the live-wrist path (=2). ApplyGloveIsolation
# opens with `if (mode <= 0) return;`, so 0 disabled the whole gloves-only model
# AND made its own GloveCensus diagnostic unreachable - the silence was read as
# "the code is dead". Absent is fine (code default 1); an explicit 0 is the bug.
if ($ini -match '(?m)^GloveIsolate=0\s*$') { throw 'GloveIsolate=0 silently disables the gloves-only model (use 1=census or 2=live wrist)' }
# The independent-hand path is what carries glove posing at all; if the shipping
# ini ever drops it, PrepareIndependentLeft returns at its first condition and no
# amount of GloveIsolate tuning can reach the mesh.
if ($ini -match '(?m)^IndependentHandsPrototype=0\s*$') { throw 'IndependentHandsPrototype=0 disables the independent-hand/glove path entirely' }
Write-Host 'PASS: no frame-rate disk log; one menu projector; sampled-shadow suppression; evidence modes off'
