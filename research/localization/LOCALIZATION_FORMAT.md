# Singularity localization format

## Confirmed sources

The installed game is under `C:\Program Files (x86)\Steam\steamapps\common\Singularity`.
`RvGame\CookedPC\Coalesced_INT.bin` is 1,524,032 bytes and has SHA-1
`ffa326e7392129e7d11b090a831bf72db9547f28`. The untouched parser round-trips
the file byte-for-byte. The container has 33 file records and 13,108 key/value
pairs. The shipped Spanish, French and Italian containers have 20 records and
8,769 pairs each.

The container is a sequence of little-endian signed 32-bit integers and
length-prefixed strings:

1. record count;
2. record name, section count;
3. section name, pair count;
4. key and value for each pair.

A positive length is a Latin-1 byte count including a terminating NUL. A
negative length is a UTF-16LE code-unit count including a terminating NUL. A
zero length represents an empty string. The implementation is in
`localization-tools/coalesced_format.py` and refuses truncated or trailing
data.

The main UI/tutorial records are in `RvGame.int`, including `RvUI`, `RvUI3D`,
`Tutorial`, `GameDialogs` and `TCR`. `Subtitles.int` contains subtitle-related
keys. `RvInput.ini`, `RvObjectives.ini` and the Unreal Engine `Core.int` record
are also present. These are confirmed string sources for the first batch; a
complete subtitle/dialogue inventory still needs the localized package export
mapping.

## UE3 package sources

CookedPC contains `*_LOC_INT.xxx` packages for single-player and multiplayer
content, with matching ESN/FRA/ITA packages. The repository's existing
`tools/uedecompress` handles the outer LZO-compressed `.xxx` container and
`tools/uepkg` is an exploratory UE3 package reader. The inner package tag is
`0x9E2A83C1`; package parsing is not yet treated as a safe repack path.

`GFxUI.xxx`, `UI_SinglePlayer_SF.xxx`, `ui_controls_pc.xxx` and
`ui_GFx_fonts.xxx` are likely Scaleform assets. Their role is recorded for
follow-up export and font inspection, but no installed package has been
modified.

## Integrity boundary

`Coalesced_INT.bin`'s original digest is embedded in the executable according
to the existing runtime audit. This branch therefore only extracts and
validates text. It does not write a replacement Coalesced file, alter
`Singularity.exe`, or install a `.xxx` package.
