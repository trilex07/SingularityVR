# Hot-path audit

Static, read-only. 36 file(s), 2288 finding(s): HIGH=919 MEDIUM=175 LOW=1194.

> Findings are leads, not proven bugs. Verify the call path and frequency before acting.

| file | line | function | match | category | frequency | severity |
|---|---|---|---|---|---|---|
| async_log.h | 26 | AsyncLogWorker | `fputc(` | file-io | helper (verify call path) | HIGH |
| async_log.h | 29 | AsyncLogWorker | `fprintf(` | file-io | helper (verify call path) | HIGH |
| d3d9.cpp | 148 | ReadLogStamp | `fopen_s(` | file-io | helper (verify call path) | HIGH |
| d3d9.cpp | 151 | ReadLogStamp | `fclose(` | file-io | helper (verify call path) | HIGH |
| d3d9.cpp | 175 | <global> | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 208 | LogInit | `fopen_s(` | file-io | helper (verify call path) | HIGH |
| d3d9.cpp | 211 | LogInit | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 221 | Log | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 229 | Log | `fopen_s(` | file-io | helper (verify call path) | HIGH |
| d3d9.cpp | 230 | Log | `vfprintf(` | file-io | helper (verify call path) | HIGH |
| d3d9.cpp | 231 | Log | `fputc(` | file-io | helper (verify call path) | HIGH |
| d3d9.cpp | 353 | CacheAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 785 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 800 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 804 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 807 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 816 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 820 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 830 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 834 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 843 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 846 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 847 | InstallXInputHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 863 | LogXInputStats | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 884 | LogXInputStats | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1073 | Hook_CalcViewRotation | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1159 | Readable | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1166 | Readable | `VirtualQuery` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1184 | NameFromIndex | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1187 | NameFromIndex | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1189 | NameFromIndex | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1200 | NameOf | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1202 | NameOf | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1206 | NameOf | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1208 | NameOf | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1360 | ScanViewRotFields | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1367 | ScanViewRotFields | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1372 | ScanViewRotFields | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1373 | ScanViewRotFields | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1375 | ScanViewRotFields | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1410 | ScanAllowed | `ScanAllowed` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1420 | FindController | `FindController` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1421 | FindController | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1425 | FindController | `ScanAllowed` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1426 | FindController | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1434 | FindController | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1436 | FindController | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1437 | FindController | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1439 | FindController | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1439 | FindController | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1446 | FindController | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1448 | FindController | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1450 | FindController | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1481 | FindCamera | `FindCamera` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1483 | FindCamera | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1493 | FindCamera | `ScanAllowed` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1494 | FindCamera | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1502 | FindCamera | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1504 | FindCamera | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1505 | FindCamera | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1507 | FindCamera | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1507 | FindCamera | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1517 | FindCamera | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1519 | FindCamera | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1521 | FindCamera | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1591 | IsLive | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1593 | IsLive | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1595 | IsLive | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1695 | ResolveWantedNames | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1716 | PropOffsetUnder | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1719 | PropOffsetUnder | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1756 | PropOffsetInChain | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1761 | PropOffsetInChain | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1780 | DumpClassProperties | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1785 | DumpClassProperties | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1790 | DumpClassProperties | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1790 | DumpClassProperties | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1791 | DumpClassProperties | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1792 | DumpClassProperties | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1795 | DumpClassProperties | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1798 | DumpClassProperties | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1799 | DumpClassProperties | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1804 | DumpClassProperties | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1807 | DumpClassProperties | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1812 | FindClassByName | `FindClassByName` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1813 | FindClassByName | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1820 | FindClassByName | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1822 | FindClassByName | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1825 | FindClassByName | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1825 | FindClassByName | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 1916 | <global> | `FindPlayerPawn` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1980 | LogEquippedClass | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1981 | LogEquippedClass | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 1983 | LogEquippedClass | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 1989 | LogEquippedClass | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 1997 | RefreshArmedState | `FindPlayerPawn` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 1998 | RefreshArmedState | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2039 | RefreshArmedState | `FindPlayerPawn` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2040 | RefreshArmedState | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2044 | RefreshArmedState | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2050 | RefreshArmedState | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2054 | RefreshArmedState | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2064 | RefreshArmedState | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2108 | FindPlayerPawn | `FindPlayerPawn` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2109 | FindPlayerPawn | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2153 | FindPlayerPawn | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2164 | FindPlayerPawn | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2166 | FindPlayerPawn | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2169 | FindPlayerPawn | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2174 | FindPlayerPawn | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2188 | FindPlayerPawn | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2224 | ReportEyeHeight | `FindPlayerPawn` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 2226 | ReportEyeHeight | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 2242 | ReportEyeHeight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 2252 | ReportEyeHeight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 2292 | ClassDerivesFrom | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 2298 | FindHudObject | `FindHudObject` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2299 | FindHudObject | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2306 | FindHudObject | `FindClassByName` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2308 | FindHudObject | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2315 | FindHudObject | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2320 | FindHudObject | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2327 | FindHudObject | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2328 | FindHudObject | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2345 | HealWouldDoSomething | `FindPlayerPawn` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 2346 | HealWouldDoSomething | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 2357 | HealWouldDoSomething | `FindHudObject` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 2358 | HealWouldDoSomething | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 2408 | SolvePropertyLayout | `FindClassByName` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2411 | SolvePropertyLayout | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2426 | SolvePropertyLayout | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2433 | SolvePropertyLayout | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2438 | SolvePropertyLayout | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2442 | SolvePropertyLayout | `FindClassByName` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2443 | SolvePropertyLayout | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2502 | ResolveGameplayOffsets | `FindPlayerPawn` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2512 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2521 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2533 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2544 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2549 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2560 | ResolveGameplayOffsets | `NameOf(` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2562 | ResolveGameplayOffsets | `Readable(` | virtualquery | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2565 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2571 | ResolveGameplayOffsets | `FindHudObject` | object-walk | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2580 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2584 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2591 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2597 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2599 | ResolveGameplayOffsets | `Log(` | log-call | MUST STAY COLD (walk/syscall helper) | HIGH |
| d3d9.cpp | 2670 | ApplyMuzzleFxSuppression | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2713 | ApplyMuzzleFxSuppression | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2722 | ApplyMuzzleFxSuppression | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2730 | ApplyMuzzleFxSuppression | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2740 | ApplyMuzzleFxSuppression | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2745 | ApplyMuzzleFxSuppression | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2758 | ApplyMuzzleFxSuppression | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 2765 | ApplyMuzzleFxSuppression | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 3813 | InitXR | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 3817 | InitXR | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 3841 | InitXR | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 3844 | InitXR | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 3891 | InitXR | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 3905 | PumpXR | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4412 | EnsureSwapchain | `delete[` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 4418 | EnsureSwapchain | `new i` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 4438 | EnsureSwapchain | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4441 | EnsureSwapchain | `delete[` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 4451 | EnsureSwapchain | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4454 | EnsureSwapchain | `new X` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 4458 | EnsureSwapchain | `new I` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 4460 | EnsureSwapchain | `delete[` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 4464 | EnsureSwapchain | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4482 | ReportResolutionRequest | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4499 | ReportResolutionRequest | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4506 | ReportResolutionRequest | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4516 | ReportResolutionRequest | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4538 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4549 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4556 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4578 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4580 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4583 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4589 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4601 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4609 | EnsureCopyResources | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4662 | MeasureCullBand | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4667 | MeasureCullBand | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4672 | MeasureCullBand | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 4725 | CopyBackbufferToD3D11 | `->GetBackBuffer(` | com-get-state | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 4743 | CopyBackbufferToD3D11 | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 4756 | CopyBackbufferToD3D11 | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 4781 | CopyBackbufferToD3D11 | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 4799 | CopyBackbufferToD3D11 | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 4813 | CopyBackbufferToD3D11 | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 5238 | ReportVrRuntimeCounters | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5240 | ReportVrRuntimeCounters | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5242 | ReportVrRuntimeCounters | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5286 | ReportHotPathSampler | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5480 | UpdateAutoPad | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5485 | UpdateAutoPad | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5740 | CpuRefreshThreads | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5757 | CpuRefreshThreads | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5776 | SampleThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5784 | SampleThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5848 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5849 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5858 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5874 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5880 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5884 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5886 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5939 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5968 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 5970 | LogThreadCpu | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 6161 | GunAlignFor | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 6617 | PanelSaveRow | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 6628 | PanelSave | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 6634 | PanelSave | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 6972 | PanelAdjust | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7530 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7600 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7676 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7683 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7695 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7718 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7820 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 7966 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8031 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8037 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8051 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8056 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8059 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8067 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8084 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8091 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8092 | InitXRInput | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 8440 | SyncXRInput | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 8477 | SyncXRInput | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 8509 | SyncXRInput | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 8542 | SyncXRInput | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 8601 | SyncXRInput | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 8612 | SyncXRInput | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 8698 | SyncXRInput | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9022 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9079 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9120 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9142 | UpdateFromHeadset | `FindCamera` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9143 | UpdateFromHeadset | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9147 | UpdateFromHeadset | `FindController` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9148 | UpdateFromHeadset | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9197 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9207 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9233 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9301 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9517 | UpdateFromHeadset | `FindCamera` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9518 | UpdateFromHeadset | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9530 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9558 | UpdateFromHeadset | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9569 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9602 | UpdateFromHeadset | `FindController` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9603 | UpdateFromHeadset | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9613 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9617 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9809 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 9940 | UpdateFromHeadset | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 10068 | Hook_SetVSConstF | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 10075 | Hook_SetVSConstF | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 10311 | Hook_SetVSConstF | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 10576 | ReportRenderTargets | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 10587 | ReportRenderTargets | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 10597 | ReportRenderTargets | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 10678 | StereoPairBody | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 10701 | StereoPairBody | `->GetPixelShaderConstantF(` | com-get-constants | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 10706 | StereoPairBody | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 10712 | StereoPairBody | `->GetVertexShaderConstantF(` | com-get-constants | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 10714 | StereoPairBody | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 11147 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11152 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11157 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11163 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11207 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11210 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11212 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11215 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11218 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11220 | DumpShaderConstUsage | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11265 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11271 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11295 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11300 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11302 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11308 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11312 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11324 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11330 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11347 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11351 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11355 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11361 | ScopeQuadDump | `->GetPixelShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| d3d9.cpp | 11366 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11378 | ScopeQuadDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11415 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11418 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11446 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11491 | FlashRegDump | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| d3d9.cpp | 11495 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11503 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11507 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11510 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11512 | FlashRegDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11529 | FireBBox | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 11605 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11611 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11631 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11647 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11651 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11654 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11660 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11667 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11671 | FireCensusDump | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11703 | CaptureDrawSig | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 11726 | CaptureDrawSig | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 11748 | CaptureDrawSig | `->GetRenderTarget(` | com-get-state | helper (verify call path) | HIGH |
| d3d9.cpp | 11811 | QuadCensusRecord | `new p` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 11811 | QuadCensusRecord | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11822 | QuadCensusRecord | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11862 | VidProbeFrameEnd | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11865 | VidProbeFrameEnd | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 11988 | Hook_DrawPrim | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 12025 | Hook_DrawPrim | `->GetPixelShaderConstantF(` | com-get-constants | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 12027 | Hook_DrawPrim | `->GetPixelShaderConstantF(` | com-get-constants | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 12591 | FgRememberArms | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 12738 | ApplyAimMethod | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13121 | PrepareIndependentLeft | `->GetVertexShaderConstantF(` | com-get-constants | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 13122 | PrepareIndependentLeft | `->GetVertexShaderConstantF(` | com-get-constants | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 13290 | FgIdentityProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13355 | FgSoloStep | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13554 | AdsCapture | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| d3d9.cpp | 13577 | AdsMask | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| d3d9.cpp | 13597 | AdsFreeze | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| d3d9.cpp | 13610 | AdsFreeze | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13755 | ReportFgRide | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13762 | ReportFgRide | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13777 | ReportFgRide | `new p` | heap-alloc | helper (verify call path) | HIGH |
| d3d9.cpp | 13777 | ReportFgRide | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13825 | NoteOfferLog | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13829 | NoteOfferLog | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13843 | NoteMissLog | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 13847 | NoteMissLog | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14032 | LogFgList | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14082 | UpdateFgLatch | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 14108 | UpdateFgLatch | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 14177 | UpdateFgLatch | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 14198 | UpdateFgLatch | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 14281 | UpdateFgLatch | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 14299 | UpdateFgLatch | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 14331 | UpdateFgLatch | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 14385 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14389 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14392 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14395 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14397 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14400 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14412 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14414 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14416 | DumpDpgProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14439 | CaptureRenderEvidence | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14440 | CaptureRenderEvidence | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14448 | CaptureRenderEvidence | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14452 | CaptureRenderEvidence | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14456 | CaptureRenderEvidence | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| d3d9.cpp | 14461 | CaptureRenderEvidence | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14468 | CaptureRenderEvidence | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14488 | CaptureShadowEvidence | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 14703 | Hook_DrawIndexedPrim | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 14709 | Hook_DrawIndexedPrim | `->GetVertexShaderConstantF(` | com-get-constants | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 14797 | Hook_DrawIndexedPrim | `->GetVertexShaderConstantF(` | com-get-constants | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 14806 | Hook_DrawIndexedPrim | `CTAB` | shader-ctab | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 14806 | Hook_DrawIndexedPrim | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 14885 | NoteFirstUserPtrDraw | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15199 | ReportXhairCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15234 | DumpUserPtrTransform | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15241 | DumpUserPtrTransform | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| d3d9.cpp | 15247 | DumpUserPtrTransform | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15257 | DumpUserPtrTransform | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15267 | DumpUserPtrTransform | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15273 | DumpUserPtrTransform | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15279 | DumpUserPtrTransform | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15288 | DumpUserPtrVertex | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 15291 | DumpUserPtrVertex | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 15320 | UserPtrQuad | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 15379 | HudStereoPair | `->GetVertexShaderConstantF(` | com-get-constants | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 15428 | HudStereoPair | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 15452 | HudStereoPair | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 15677 | IsVideoQuad | `->GetRenderTarget(` | com-get-state | helper (verify call path) | HIGH |
| d3d9.cpp | 15715 | SpriteQuadNeedsSplit | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 15747 | DrawSpriteQuadPerEye | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 15783 | DrawVideoQuadPerEye | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 15967 | Hook_DrawPrimUP | `Log(` | log-call | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 16090 | Hook_DrawIndexedPrimUP | `Readable(` | virtualquery | per-draw (x1000s/frame) | HIGH |
| d3d9.cpp | 16243 | Hook_CreateQuery | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16248 | Hook_CreateQuery | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16368 | StageAdd | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16483 | FlushShadow | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16551 | Hook_TexGetSurfaceLevel | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16708 | Hook_CreateTexture | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16724 | Hook_CreateTexture | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16808 | PatchTextureVTableOnce | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16819 | PatchCubeVTableOnce | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16828 | PatchVolVTableOnce | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16832 | ReportPoolTranslation | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16844 | ReportPoolTranslation | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16860 | RefreshCameraPosition | `FindCamera` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 16877 | RefreshCameraPosition | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 16885 | RefreshCameraPosition | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16889 | RefreshCameraPosition | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16896 | RefreshCameraPosition | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16924 | ApplyVrFov | `FindCamera` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 16969 | ApplyVrFov | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16971 | ApplyVrFov | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16973 | ApplyVrFov | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16980 | ApplyVrFov | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16981 | ApplyVrFov | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16982 | ApplyVrFov | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 16990 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17000 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17018 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17023 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17035 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17043 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17051 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17058 | ApplyVrFov | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17085 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17088 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17090 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17094 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17098 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17099 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17100 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17101 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17122 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17128 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17134 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17136 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17151 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17158 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17161 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17165 | ReportScan | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17284 | ScriptFuncNameIdx | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17294 | ScriptFuncNameIdx | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17297 | ScriptFuncNameIdx | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17310 | ScriptFuncNameIdx | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17313 | ScriptFuncNameIdx | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17321 | ScriptFuncNameIdx | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17324 | ScriptFuncNameIdx | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17489 | NoteNested | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17542 | RewriteSetAimParams | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17545 | RewriteSetAimParams | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17552 | RewriteSetAimParams | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17580 | RewriteSetAimParams | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17645 | FindTextSection | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17649 | FindTextSection | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17659 | FindTextSection | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17666 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17667 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17667 | FindProcessInternal | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17667 | FindProcessInternal | `GObjects` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17670 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17670 | FindProcessInternal | `GObjects` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17687 | FindProcessInternal | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17689 | FindProcessInternal | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17691 | FindProcessInternal | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17693 | FindProcessInternal | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17711 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17711 | FindProcessInternal | `GObjects` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17713 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17727 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17734 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17738 | FindProcessInternal | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17767 | ProbeTraceNative | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17768 | ProbeTraceNative | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17779 | ProbeTraceNative | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17781 | ProbeTraceNative | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17783 | ProbeTraceNative | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17784 | ProbeTraceNative | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17785 | ProbeTraceNative | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17793 | ProbeTraceNative | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17793 | ProbeTraceNative | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 17795 | ProbeTraceNative | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17807 | ProbeTraceNative | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17814 | ProbeTraceNative | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17816 | ProbeTraceNative | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17821 | ProbeTraceNative | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17915 | Hook_LineCheck | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17921 | Hook_LineCheck | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17935 | Hook_LineCheck | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17939 | Hook_LineCheck | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 17957 | Hook_LineCheck | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 17999 | Hook_LineCheck | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18008 | Hook_LineCheck | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18012 | Hook_LineCheck | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18053 | Hook_SingleLineCheck | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18083 | Hook_SingleLineCheck | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18099 | InstallSingleLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18099 | InstallSingleLineCheckHook | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18103 | InstallSingleLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18108 | InstallSingleLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18110 | InstallSingleLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18111 | InstallSingleLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18118 | InstallLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18118 | InstallLineCheckHook | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18124 | InstallLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18129 | InstallLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18131 | InstallLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18132 | InstallLineCheckHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18180 | EnsureRouteTwoHooks | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18193 | RepointScriptHook | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18195 | RepointScriptHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18199 | RepointScriptHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18204 | RepointScriptHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18212 | RepointScriptHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18221 | RepointScriptHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18236 | RepointScriptHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18242 | ResolveScriptNames | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18242 | ResolveScriptNames | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18264 | ResolveScriptNames | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18266 | ResolveScriptNames | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18269 | ResolveScriptNames | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18313 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18341 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18342 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18349 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18379 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18383 | Hook_ProcessEvent | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 18387 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18523 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18526 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18534 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18537 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18550 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18553 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18560 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18567 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18683 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18685 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18685 | Hook_ProcessEvent | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 18686 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18700 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18702 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18702 | Hook_ProcessEvent | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 18703 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18732 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18738 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18744 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18797 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18814 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18877 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18882 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18910 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18918 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18928 | Hook_ProcessEvent | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 18929 | Hook_ProcessEvent | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18938 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18941 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18990 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18993 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 18995 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19000 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19004 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19006 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19009 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19013 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19015 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19026 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19030 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19049 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19053 | DumpProcessEventCensus | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19061 | WriteAimAtInputPoll | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 19081 | InstallProcessEventHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19092 | InstallViewHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19097 | InstallViewHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19099 | InstallViewHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19100 | InstallViewHook | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19162 | SetVrMode | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19435 | ClearRects | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19580 | DrawVrPanel | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19591 | DrawVrPanel | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19813 | DrawStateReadout | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 19988 | DrawWristHealth | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 19989 | DrawWristHealth | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 20093 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20099 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20122 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20125 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20126 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20127 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20128 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20133 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20140 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20158 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20167 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20185 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20189 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20210 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20216 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20223 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20229 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20249 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20253 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20270 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20287 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20301 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20313 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20340 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20354 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20417 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20418 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20422 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20432 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20435 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20451 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20464 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20475 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20490 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20505 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20516 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20532 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20541 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20569 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20596 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20613 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20638 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20647 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20679 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20713 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20767 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20779 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20791 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20819 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20824 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20847 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20850 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20877 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20889 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20900 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20917 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20930 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20945 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20959 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20973 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20981 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20983 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20992 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 20999 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21018 | Hook_Present | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21019 | Hook_Present | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21034 | Hook_Present | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21036 | Hook_Present | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21040 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21152 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21155 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21241 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21267 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21277 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21291 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21300 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21342 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21348 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21364 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21380 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21393 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21401 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21407 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21420 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21428 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21432 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21443 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21455 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21472 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21488 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21519 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21532 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21536 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21545 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21553 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21571 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21595 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21613 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21621 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21641 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21655 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21668 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21698 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21718 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21718 | Hook_Present | `GObjects` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21727 | Hook_Present | `new 5` | heap-alloc | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21727 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21733 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21735 | Hook_Present | `GObjects` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21741 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21756 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21770 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21785 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21795 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21804 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21804 | Hook_Present | `GObjects` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21838 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21853 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21890 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21897 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21903 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21911 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21938 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21943 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21959 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21965 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21977 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21987 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 21991 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22001 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22017 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22022 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22038 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22070 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22074 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22091 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22135 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22143 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22172 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22190 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22196 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22200 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22201 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22203 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22205 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22237 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22249 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22280 | Hook_Present | `FindController` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22298 | Hook_Present | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22392 | Hook_Present | `FindCamera` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22393 | Hook_Present | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22398 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22401 | Hook_Present | `FindCamera` | object-walk | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22433 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22510 | Hook_Present | `Readable(` | virtualquery | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22640 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22679 | Hook_Present | `Log(` | log-call | per-frame (~90-120/s) | HIGH |
| d3d9.cpp | 22743 | Hook_DInputDeviceCreate | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22766 | InstallDirectInputStartupProbe | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22802 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22806 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22807 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22826 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22902 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22910 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22919 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22932 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22943 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22984 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22989 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 22999 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23011 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23023 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23042 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23046 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23049 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23062 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23074 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23081 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23091 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23100 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23107 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23113 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23123 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23136 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23144 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23149 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23159 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23179 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23193 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23205 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23214 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23226 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23236 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23251 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23251 | LoadIniSettings | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| d3d9.cpp | 23251 | LoadIniSettings | `GObjects` | object-walk | helper (verify call path) | HIGH |
| d3d9.cpp | 23259 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23270 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23274 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23279 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23291 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23334 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23345 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23355 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23358 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23364 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23365 | LoadIniSettings | `CTAB` | shader-ctab | helper (verify call path) | HIGH |
| d3d9.cpp | 23365 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23367 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23379 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23385 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23393 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23404 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23421 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23432 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23442 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23447 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23459 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23465 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23480 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23497 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23514 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23525 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23535 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23545 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23555 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23566 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23581 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23590 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23595 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23600 | LoadIniSettings | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23612 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23619 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23640 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23641 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23646 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23647 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23658 | Hook_CreateDevice | `->GetBackBuffer(` | com-get-state | helper (verify call path) | HIGH |
| d3d9.cpp | 23660 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23677 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23684 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23691 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23707 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23710 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23717 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23741 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23746 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23750 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23763 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23767 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23768 | Hook_CreateDevice | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23829 | Direct3DCreate9 | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23831 | Direct3DCreate9 | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23864 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23869 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23875 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23878 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23881 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23885 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23890 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23896 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23899 | InstallAutoResolution | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23922 | PhysXPreflight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23928 | PhysXPreflight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23935 | PhysXPreflight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23936 | PhysXPreflight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23937 | PhysXPreflight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23938 | PhysXPreflight | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23965 | DllMain | `Log(` | log-call | helper (verify call path) | HIGH |
| d3d9.cpp | 23968 | DllMain | `Log(` | log-call | helper (verify call path) | HIGH |
| diagnostic_trace.h | 112 | DiagLog | `fopen_s(` | file-io | helper (verify call path) | HIGH |
| diagnostic_trace.h | 113 | DiagLog | `vfprintf(` | file-io | helper (verify call path) | HIGH |
| diagnostic_trace.h | 119 | DiagReadSteamSettings | `CreateFileA(` | file-io | helper (verify call path) | HIGH |
| gpu_copy_timing.h | 40 | Begin | `Log(` | log-call | helper (verify call path) | HIGH |
| input_wmi_cache.h | 44 | InstallInputClassificationCache | `Log(` | log-call | helper (verify call path) | HIGH |
| input_wmi_cache.h | 50 | InstallInputClassificationCache | `Log(` | log-call | helper (verify call path) | HIGH |
| input_wmi_cache.h | 58 | InstallInputClassificationCache | `Log(` | log-call | helper (verify call path) | HIGH |
| input_wmi_cache.h | 60 | InstallInputClassificationCache | `Log(` | log-call | helper (verify call path) | HIGH |
| input_wmi_cache.h | 66 | ReportInputClassificationCache | `Log(` | log-call | helper (verify call path) | HIGH |
| menu_draw_trace.h | 31 | MenuDrawTraceFlush | `Log(` | log-call | helper (verify call path) | HIGH |
| menu_draw_trace.h | 34 | MenuDrawTraceFlush | `Log(` | log-call | helper (verify call path) | HIGH |
| menu_draw_trace.h | 36 | MenuDrawTraceFlush | `Log(` | log-call | helper (verify call path) | HIGH |
| menu_draw_trace.h | 38 | MenuDrawTraceFlush | `Log(` | log-call | helper (verify call path) | HIGH |
| menu_draw_trace.h | 40 | MenuDrawTraceFlush | `Log(` | log-call | helper (verify call path) | HIGH |
| menu_draw_trace.h | 67 | MenuDrawTraceRecord | `->GetRenderTarget(` | com-get-state | helper (verify call path) | HIGH |
| menu_draw_trace.h | 80 | MenuDrawTraceRecord | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| missing_mouse.h | 86 | Install | `Log(` | log-call | helper (verify call path) | HIGH |
| missing_mouse.h | 90 | Install | `Log(` | log-call | helper (verify call path) | HIGH |
| missing_mouse.h | 99 | Install | `Log(` | log-call | helper (verify call path) | HIGH |
| missing_mouse.h | 102 | Install | `Log(` | log-call | helper (verify call path) | HIGH |
| native_ui_input.h | 22 | ReadPointer | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 43 | UpdateMenu | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| native_ui_input.h | 44 | UpdateMenu | `NameOf(` | object-walk | helper (verify call path) | HIGH |
| native_ui_input.h | 49 | UpdateMenu | `Log(` | log-call | helper (verify call path) | HIGH |
| native_ui_input.h | 58 | Coordinates | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 73 | BindViewport | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 76 | BindViewport | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 78 | BindViewport | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 82 | BindViewport | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 94 | BindViewport | `Log(` | log-call | helper (verify call path) | HIGH |
| native_ui_input.h | 101 | Drain | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 103 | Drain | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 119 | Drain | `Log(` | log-call | helper (verify call path) | HIGH |
| native_ui_input.h | 133 | Install | `Log(` | log-call | helper (verify call path) | HIGH |
| native_ui_input.h | 136 | Install | `Log(` | log-call | helper (verify call path) | HIGH |
| native_ui_input.h | 136 | Install | `Readable(` | virtualquery | helper (verify call path) | HIGH |
| native_ui_input.h | 139 | Install | `Log(` | log-call | helper (verify call path) | HIGH |
| psvr2_client.h | 186 | Worker | `Sleep(` | sleep | helper (verify call path) | HIGH |
| psvr2_client.h | 190 | Worker | `WriteFile(` | file-io | helper (verify call path) | HIGH |
| psvr2_client.h | 216 | Worker | `WriteFile(` | file-io | helper (verify call path) | HIGH |
| psvr2_client.h | 218 | Worker | `WriteFile(` | file-io | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 21 | LoadVrPromptAtlas | `CreateFileA(` | file-io | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 22 | LoadVrPromptAtlas | `Log(` | log-call | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 24 | LoadVrPromptAtlas | `malloc(` | heap-alloc | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 26 | LoadVrPromptAtlas | `free(` | heap-alloc | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 29 | LoadVrPromptAtlas | `free(` | heap-alloc | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 32 | LoadVrPromptAtlas | `free(` | heap-alloc | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 32 | LoadVrPromptAtlas | `Log(` | log-call | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 34 | LoadVrPromptAtlas | `free(` | heap-alloc | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 37 | LoadVrPromptAtlas | `free(` | heap-alloc | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 38 | LoadVrPromptAtlas | `Log(` | log-call | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 131 | SyncVrMenuCaptureState | `Log(` | log-call | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 151 | CaptureVrMenuTarget | `->GetVertexShaderConstantF(` | com-get-constants | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 152 | CaptureVrMenuTarget | `->GetRenderTarget(` | com-get-state | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 166 | CaptureVrMenuTarget | `Log(` | log-call | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 203 | DrawVrMenuPointer | `->GetBackBuffer(` | com-get-state | helper (verify call path) | HIGH |
| vr_menu_overlay.h | 207 | DrawVrMenuPointer | `->GetRenderTarget(` | com-get-state | helper (verify call path) | HIGH |
| async_log.h | 16 | AsyncLogWorker | `WaitForSingleObject` | lock | helper (verify call path) | MEDIUM |
| async_log.h | 21 | AsyncLogWorker | `AcquireSRWLock` | lock | helper (verify call path) | MEDIUM |
| async_log.h | 50 | AsyncLogEnqueue | `AcquireSRWLock` | lock | helper (verify call path) | MEDIUM |
| async_thread_snapshot.h | 15 | ThreadSnapshotWorker | `WaitForMultipleObjects` | lock | helper (verify call path) | MEDIUM |
| async_thread_snapshot.h | 26 | ThreadSnapshotWorker | `AcquireSRWLock` | lock | helper (verify call path) | MEDIUM |
| async_thread_snapshot.h | 55 | AsyncThreadSnapshotRead | `AcquireSRWLock` | lock | helper (verify call path) | MEDIUM |
| async_thread_snapshot.h | 66 | AsyncThreadSnapshotShutdown | `WaitForSingleObject` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 227 | Log | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 3830 | InitXR | `xrEnumerateViewConfigurationViews(` | openxr-call | helper (verify call path) | MEDIUM |
| d3d9.cpp | 3836 | InitXR | `xrEnumerateViewConfigurationViews(` | openxr-call | helper (verify call path) | MEDIUM |
| d3d9.cpp | 3864 | InitXR | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 3866 | InitXR | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 3871 | InitXR | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 3873 | InitXR | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 3898 | PumpXR | `xrPollEvent(` | openxr-call | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4417 | EnsureSwapchain | `xrEnumerateSwapchainFormats(` | openxr-call | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4419 | EnsureSwapchain | `xrEnumerateSwapchainFormats(` | openxr-call | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4453 | EnsureSwapchain | `xrEnumerateSwapchainImages(` | openxr-call | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4456 | EnsureSwapchain | `xrEnumerateSwapchainImages(` | openxr-call | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4526 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4530 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4568 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4569 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4570 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4579 | EnsureCopyResources | `->GetSurfaceLevel(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4587 | EnsureCopyResources | `->GetDesc(` | com-getdesc | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4593 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4594 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4595 | EnsureCopyResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 4714 | CopyBackbufferToD3D11 | `CopyBackbufferToD3D11(` | xr-submit | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 4727 | CopyBackbufferToD3D11 | `->GetDesc(` | com-getdesc | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 4728 | CopyBackbufferToD3D11 | `->Release(` | com-refcount | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 4738 | CopyBackbufferToD3D11 | `->Release(` | com-refcount | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 4807 | CopyBackbufferToD3D11 | `GetRenderTargetData(` | xr-submit | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 4809 | CopyBackbufferToD3D11 | `->Release(` | com-refcount | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 5253 | ReportHotPathSampler | `append("` | string-alloc | helper (verify call path) | MEDIUM |
| d3d9.cpp | 5273 | ReportHotPathSampler | `append("` | string-alloc | helper (verify call path) | MEDIUM |
| d3d9.cpp | 5285 | ReportHotPathSampler | `append("` | string-alloc | helper (verify call path) | MEDIUM |
| d3d9.cpp | 8124 | SyncXRInput | `xrSyncActions(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8172 | SyncXRInput | `xrGetActionStateVector2f(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8184 | SyncXRInput | `xrGetActionStateFloat(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8194 | SyncXRInput | `xrGetActionStateBoolean(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8205 | SyncXRInput | `xrGetActionStateBoolean(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8319 | SyncXRInput | `xrLocateSpace(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8336 | SyncXRInput | `atan2f(` | ik-solve | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8340 | SyncXRInput | `atan2f(` | ik-solve | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8372 | SyncXRInput | `xrGetActionStateBoolean(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8745 | SyncXRInput | `xrGetActionStateFloat(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8826 | SyncXRInput | `xrGetActionStateBoolean(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8841 | SyncXRInput | `xrGetActionStateBoolean(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 8972 | UpdateFromHeadset | `xrWaitFrame(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9039 | UpdateFromHeadset | `xrBeginFrame(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9051 | UpdateFromHeadset | `xrLocateViews(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9065 | UpdateFromHeadset | `atan2f(` | ik-solve | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9070 | UpdateFromHeadset | `atan2f(` | ik-solve | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9442 | UpdateFromHeadset | `atan2f(` | ik-solve | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9822 | UpdateFromHeadset | `CopyBackbufferToD3D11(` | xr-submit | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9965 | UpdateFromHeadset | `xrEndFrame(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 9965 | UpdateFromHeadset | `xrEndFrame(` | xr-submit | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 10535 | Hook_SetRenderTarget | `->GetDesc(` | com-getdesc | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 10565 | Hook_SetRenderTarget | `->GetDesc(` | com-getdesc | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 11270 | ScopeQuadDump | `->GetPixelShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11306 | ScopeQuadDump | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11311 | ScopeQuadDump | `->GetVertexShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11313 | ScopeQuadDump | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11328 | ScopeQuadDump | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11336 | ScopeQuadDump | `->GetStreamSource(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11357 | ScopeQuadDump | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11374 | ScopeQuadDump | `->GetTexture(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11382 | ScopeQuadDump | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11394 | FlashRegDump | `->GetVertexShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11424 | FlashRegDump | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11739 | CaptureDrawSig | `->GetTexture(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11746 | CaptureDrawSig | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11750 | CaptureDrawSig | `->GetDesc(` | com-getdesc | helper (verify call path) | MEDIUM |
| d3d9.cpp | 11751 | CaptureDrawSig | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 13275 | FgIdentityProbe | `->GetStreamSource(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 13276 | FgIdentityProbe | `->GetIndices(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 13300 | FgIdentityProbe | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 13301 | FgIdentityProbe | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 13857 | BoundTexId | `->GetTexture(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 13859 | BoundTexId | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14428 | CaptureRenderEvidence | `->GetVertexShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14429 | CaptureRenderEvidence | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14436 | CaptureRenderEvidence | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14469 | CaptureRenderEvidence | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14471 | CaptureRenderEvidence | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14480 | CaptureShadowEvidence | `->GetTexture(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14487 | CaptureShadowEvidence | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14499 | ReleaseFpCasterCache | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14500 | ReleaseFpCasterCache | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14515 | TrackFpCaster | `->GetStreamSource(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14516 | TrackFpCaster | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 14525 | TrackFpCaster | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 15228 | DumpUserPtrTransform | `->GetVertexShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 15238 | DumpUserPtrTransform | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 15667 | IsVideoQuad | `->GetTexture(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 15674 | IsVideoQuad | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 15679 | IsVideoQuad | `->GetDesc(` | com-getdesc | helper (verify call path) | MEDIUM |
| d3d9.cpp | 15680 | IsVideoQuad | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16356 | StageAdd | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16388 | StageRemove | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16391 | StageRemove | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16489 | Hook_TexLockRect | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16508 | Hook_TexUnlockRect | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16544 | Hook_TexGetSurfaceLevel | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16560 | Hook_CubeLockRect | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16570 | Hook_CubeUnlockRect | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16590 | Hook_VolLockBox | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 16600 | Hook_VolUnlockBox | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| d3d9.cpp | 17415 | <global> | `IsMenuOpen` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 17898 | RotateTrace | `atan2f(` | ik-solve | helper (verify call path) | MEDIUM |
| d3d9.cpp | 19906 | DrawLaserSight | `->GetVertexShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 19907 | DrawLaserSight | `->GetPixelShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| d3d9.cpp | 19973 | DrawLaserSight | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 19974 | DrawLaserSight | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 19982 | ReleaseWristResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| d3d9.cpp | 21432 | Hook_Present | `HudArmSticky` | menu-classify | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 21531 | Hook_Present | `xrEnumerateBoundSourcesForAction(` | openxr-call | per-frame (~90-120/s) | MEDIUM |
| d3d9.cpp | 23175 | LoadIniSettings | `HudStereo` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23179 | LoadIniSettings | `HudStereo` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23258 | LoadIniSettings | `HudArmSticky` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23259 | LoadIniSettings | `HudArmSticky` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23270 | LoadIniSettings | `GFx` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23364 | LoadIniSettings | `GFx` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23595 | LoadIniSettings | `HudStereo` | menu-classify | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23658 | Hook_CreateDevice | `->GetDesc(` | com-getdesc | helper (verify call path) | MEDIUM |
| d3d9.cpp | 23658 | Hook_CreateDevice | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| diagnostic_trace.h | 86 | DiagFinish | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| diagnostic_trace.h | 133 | DiagWorker | `WaitForSingleObject` | lock | helper (verify call path) | MEDIUM |
| diagnostic_trace.h | 137 | DiagWorker | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| diagnostic_trace.h | 202 | DiagDisable | `WaitForSingleObject` | lock | helper (verify call path) | MEDIUM |
| gpu_copy_timing.h | 9 | Reset | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| gpu_copy_timing.h | 22 | Begin | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| input_classification_cache.h | 19 | Query | `AcquireSRWLock` | lock | helper (verify call path) | MEDIUM |
| input_classification_cache.h | 36 | Query | `AcquireSRWLock` | lock | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 67 | MenuDrawTraceRecord | `->GetVertexShader(` | com-get-resource | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 68 | MenuDrawTraceRecord | `->GetTexture(` | com-get-resource | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 69 | MenuDrawTraceRecord | `->GetStreamSource(` | com-get-resource | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 70 | MenuDrawTraceRecord | `->GetIndices(` | com-get-resource | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 72 | MenuDrawTraceRecord | `->GetDesc(` | com-getdesc | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 72 | MenuDrawTraceRecord | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 75 | MenuDrawTraceRecord | `->GetSurfaceLevel(` | com-get-resource | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 75 | MenuDrawTraceRecord | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| menu_draw_trace.h | 77 | MenuDrawTraceRecord | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| psvr2_client.h | 63 | Start | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 89 | Submit | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 100 | Stop | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 112 | Stop | `WaitForSingleObject` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 115 | Stop | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 122 | GetStats | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 137 | Worker | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 153 | Worker | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 169 | Worker | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 194 | Worker | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| psvr2_client.h | 205 | Worker | `EnterCriticalSection` | lock | helper (verify call path) | MEDIUM |
| recenter.h | 72 | YawOf | `atan2(` | ik-solve | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 34 | LoadVrPromptAtlas | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 133 | SyncVrMenuCaptureState | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 134 | SyncVrMenuCaptureState | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 153 | CaptureVrMenuTarget | `->GetDesc(` | com-getdesc | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 155 | CaptureVrMenuTarget | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 166 | CaptureVrMenuTarget | `GFx` | menu-classify | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 174 | CaptureVrMenuTarget | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 175 | CaptureVrMenuTarget | `->AddRef(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 179 | CaptureVrMenuTarget | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 182 | CaptureVrMenuTarget | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 187 | ReleaseVrMenuResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 188 | ReleaseVrMenuResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 190 | ReleaseVrMenuResources | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 204 | DrawVrMenuPointer | `->GetDesc(` | com-getdesc | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 205 | DrawVrMenuPointer | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 208 | DrawVrMenuPointer | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 209 | DrawVrMenuPointer | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| vr_menu_overlay.h | 276 | DrawVrMenuPointer | `->Release(` | com-refcount | helper (verify call path) | MEDIUM |
| async_log.h | 28 | AsyncLogWorker | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| async_log.h | 34 | AsyncLogInit | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| async_log.h | 41 | AsyncLogInit | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| async_log.h | 44 | AsyncLogInit | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| async_log.h | 45 | AsyncLogInit | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| async_log.h | 48 | AsyncLogEnqueue | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| async_log.h | 51 | AsyncLogEnqueue | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 257 | Now | `Now()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 259 | MsSince | `MsSince(` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 261 | MsSince | `QueryPerformanceFrequency` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 262 | MsSince | `QueryPerformanceCounter` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 504 | Hook_XInputSetState | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 505 | Hook_XInputSetState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 507 | Hook_XInputSetState | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 508 | Hook_XInputSetState | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 671 | Hook_Sleep | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 672 | Hook_Sleep | `InterlockedAdd` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 673 | Hook_Sleep | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 696 | Hook_XInputGetState | `Now()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 714 | Hook_XInputGetState | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 715 | Hook_XInputGetState | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 729 | Hook_XInputGetState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 740 | Hook_XInputGetState | `MsSince(` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 745 | Hook_XInputGetState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 746 | Hook_XInputGetState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 747 | Hook_XInputGetState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 764 | Hook_XInputGetState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 770 | Hook_XInputGetState | `MsSince(` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 793 | InstallXInputHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 795 | InstallXInputHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 855 | LogXInputStats | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 857 | LogXInputStats | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 866 | LogXInputStats | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 867 | LogXInputStats | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 868 | LogXInputStats | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 869 | LogXInputStats | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 870 | LogXInputStats | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 871 | LogXInputStats | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 872 | LogXInputStats | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 895 | ClearPad | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 896 | ClearPad | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 897 | ClearPad | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 955 | ApplyPadTestOverrides | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 956 | ApplyPadTestOverrides | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 967 | ApplyPadTestOverrides | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 968 | ApplyPadTestOverrides | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 973 | ApplyPadTestOverrides | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 974 | ApplyPadTestOverrides | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1056 | InGameplay | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1075 | Hook_CalcViewRotation | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1077 | Hook_CalcViewRotation | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1354 | ScanViewRotFields | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1355 | ScanViewRotFields | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1366 | ScanViewRotFields | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1371 | ScanViewRotFields | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 1411 | ScanAllowed | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 2032 | RefreshArmedState | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2033 | RefreshArmedState | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2043 | RefreshArmedState | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2053 | RefreshArmedState | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2063 | RefreshArmedState | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2147 | FindPlayerPawn | `GetTickCount64()` | timing-qpc | MUST STAY COLD (walk/syscall helper) | LOW |
| d3d9.cpp | 2159 | FindPlayerPawn | `QueryPerformanceFrequency` | timing-qpc | MUST STAY COLD (walk/syscall helper) | LOW |
| d3d9.cpp | 2160 | FindPlayerPawn | `QueryPerformanceCounter` | timing-qpc | MUST STAY COLD (walk/syscall helper) | LOW |
| d3d9.cpp | 2179 | FindPlayerPawn | `QueryPerformanceCounter` | timing-qpc | MUST STAY COLD (walk/syscall helper) | LOW |
| d3d9.cpp | 2222 | ReportEyeHeight | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 2246 | ReportEyeHeight | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 2403 | SolvePropertyLayout | `GetTickCount64()` | timing-qpc | MUST STAY COLD (walk/syscall helper) | LOW |
| d3d9.cpp | 2486 | ResolveGameplayOffsets | `InterlockedCompareExchange` | interlocked-hot | MUST STAY COLD (walk/syscall helper) | LOW |
| d3d9.cpp | 2489 | ResolveGameplayOffsets | `GetTickCount64()` | timing-qpc | MUST STAY COLD (walk/syscall helper) | LOW |
| d3d9.cpp | 2659 | ApplyMuzzleFxSuppression | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2663 | ApplyMuzzleFxSuppression | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2705 | ApplyMuzzleFxSuppression | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2712 | ApplyMuzzleFxSuppression | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2752 | ApplyMuzzleFxSuppression | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 2764 | ApplyMuzzleFxSuppression | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 3125 | MetresToUU | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4113 | Hook_SetVS | `g_boundShaderLayout.shadow` | shadow-classify | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4409 | EnsureSwapchain | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4655 | MeasureCullBand | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4666 | MeasureCullBand | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4671 | MeasureCullBand | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4683 | StampCaptureState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4684 | StampCaptureState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4695 | StampCaptureState | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4734 | CopyBackbufferToD3D11 | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4739 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4753 | CopyBackbufferToD3D11 | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4756 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4759 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4791 | CopyBackbufferToD3D11 | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4799 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4802 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4806 | CopyBackbufferToD3D11 | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4808 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4838 | CopyBackbufferToD3D11 | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4841 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4843 | CopyBackbufferToD3D11 | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4862 | CopyBackbufferToD3D11 | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 4917 | CutsceneMode | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4918 | CutsceneMode | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4926 | SetCutsceneMode | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4927 | SetCutsceneMode | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 4975 | VrModeOn | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5231 | VrMenuStateToken | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5259 | ReportHotPathSampler | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5260 | ReportHotPathSampler | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5261 | ReportHotPathSampler | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5262 | ReportHotPathSampler | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5263 | ReportHotPathSampler | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5264 | ReportHotPathSampler | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5474 | UpdateAutoPad | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5477 | UpdateAutoPad | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5483 | UpdateAutoPad | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5579 | StateBucketIndex | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5580 | StateBucketIndex | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5792 | SampleThreadCpu | `Now()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 5793 | SampleThreadCpu | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 5794 | SampleThreadCpu | `MsSince(` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 6052 | DebugKey | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6172 | GunAnchorInForce | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6224 | GunViewFrame | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6438 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6442 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6490 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6494 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6505 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6509 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6513 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6582 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6588 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6592 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6596 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6600 | PanelSaveRow | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6647 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6652 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6667 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6679 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6711 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6720 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6729 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6831 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6839 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6846 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6868 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6874 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6881 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6888 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6902 | PanelRowText | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6916 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6917 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6955 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6956 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6959 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6962 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6967 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6970 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6985 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6986 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6989 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 6998 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7005 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7011 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7018 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7026 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7027 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7028 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7047 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7050 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7051 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7057 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7073 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7082 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7090 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7091 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7092 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7093 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7102 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7103 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7104 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7116 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7120 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7123 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7135 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7142 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7153 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7156 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7162 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7165 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7171 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7174 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7178 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7181 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7185 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7186 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7193 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7194 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7212 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7213 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7222 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7236 | PanelAdjust | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7237 | PanelAdjust | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7541 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7625 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7629 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7631 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7634 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7635 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7636 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7637 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7638 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7639 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7640 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7649 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7651 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7653 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7656 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7663 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7665 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7667 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7669 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7670 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7674 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7681 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7682 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 7692 | InitXRInput | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 8112 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8113 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8114 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8121 | SyncXRInput | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8127 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8140 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8143 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8144 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8145 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8149 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8150 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8210 | SyncXRInput | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8222 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8235 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8309 | SyncXRInput | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8321 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8387 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8414 | SyncXRInput | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8419 | SyncXRInput | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8434 | SyncXRInput | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8436 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8437 | SyncXRInput | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8439 | SyncXRInput | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8446 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8466 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8467 | SyncXRInput | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8468 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8469 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8470 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8471 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8472 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8473 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8478 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8498 | SyncXRInput | `QueryPerformanceCounter` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8499 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8501 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8502 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8535 | SyncXRInput | `QueryPerformanceCounter` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8536 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8537 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8538 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8552 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8558 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8594 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8598 | SyncXRInput | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8605 | SyncXRInput | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8606 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8607 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8608 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8609 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8610 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8611 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8631 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8665 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8668 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8669 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8712 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8713 | SyncXRInput | `QueryPerformanceCounter` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8736 | SyncXRInput | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8861 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8872 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8873 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8878 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8883 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8887 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8888 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8894 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8902 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8903 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8904 | SyncXRInput | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8914 | SyncXRInput | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8916 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8917 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8923 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8924 | SyncXRInput | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8958 | SyncXRInput | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8970 | UpdateFromHeadset | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8983 | UpdateFromHeadset | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 8996 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9011 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9038 | UpdateFromHeadset | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9041 | UpdateFromHeadset | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9055 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9072 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9073 | UpdateFromHeadset | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9232 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9244 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9245 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9298 | UpdateFromHeadset | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9306 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9364 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9365 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9379 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9394 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9396 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9398 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9400 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9416 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9417 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9418 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9427 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9429 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9431 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9432 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9434 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9445 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9446 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9453 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9454 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9455 | UpdateFromHeadset | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9566 | UpdateFromHeadset | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9573 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9657 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9662 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9719 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9746 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9747 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9772 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9781 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9782 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9801 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9850 | UpdateFromHeadset | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9854 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9860 | UpdateFromHeadset | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9881 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9920 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9953 | UpdateFromHeadset | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9963 | UpdateFromHeadset | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 9969 | UpdateFromHeadset | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10013 | Hook_SetVSConstF | `g_boundShaderLayout.shadow` | shadow-classify | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10018 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10020 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10031 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10032 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10033 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10080 | Hook_SetVSConstF | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10081 | Hook_SetVSConstF | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10090 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10091 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10118 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10119 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10189 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10205 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10210 | Hook_SetVSConstF | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10224 | Hook_SetVSConstF | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10273 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10305 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10306 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10310 | Hook_SetVSConstF | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10357 | Hook_SetVSConstF | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10538 | Hook_SetRenderTarget | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10539 | Hook_SetRenderTarget | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10554 | Hook_SetRenderTarget | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 10639 | StereoPair | `Now()` | timing-qpc | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10641 | StereoPair | `MsSince(` | timing-qpc | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10653 | StereoPairBody | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10672 | StereoPairBody | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10727 | StereoPairBody | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10728 | StereoPairBody | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10734 | StereoPairBody | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10734 | StereoPairBody | `g_boundShaderLayout.shadow` | shadow-classify | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10739 | StereoPairBody | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10785 | StereoPairBody | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10791 | StereoPairBody | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10800 | StereoPairBody | `->SetScissorRect(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10833 | StereoPairBody | `->SetScissorRect(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10859 | StereoPairBody | `->SetScissorRect(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 10860 | StereoPairBody | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11556 | FireCensusRecord | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 11662 | FireCensusDump | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 11663 | FireCensusDump | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 11857 | VidProbeFrameEnd | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 11876 | Hook_DrawPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11879 | Hook_DrawPrim | `g_boundShaderLayout.shadow` | shadow-classify | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11900 | Hook_DrawPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11905 | Hook_DrawPrim | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11906 | Hook_DrawPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11950 | Hook_DrawPrim | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11951 | Hook_DrawPrim | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11953 | Hook_DrawPrim | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11959 | Hook_DrawPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11960 | Hook_DrawPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 11996 | Hook_DrawPrim | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12035 | Hook_DrawPrim | `->SetViewport(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12056 | Hook_DrawPrim | `->SetPixelShaderConstantF(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12060 | Hook_DrawPrim | `->SetPixelShaderConstantF(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12067 | Hook_DrawPrim | `->SetPixelShaderConstantF(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12068 | Hook_DrawPrim | `->SetPixelShaderConstantF(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12069 | Hook_DrawPrim | `->SetViewport(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12070 | Hook_DrawPrim | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12071 | Hook_DrawPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12072 | Hook_DrawPrim | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 12242 | RodeLearn | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12244 | RodeLearn | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12246 | RodeLearn | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12247 | RodeLearn | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12426 | HealWindowActive | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12428 | HealWindowActive | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 12429 | HealWindowActive | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12430 | HealWindowActive | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12459 | MeleeWindowActive | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12461 | MeleeWindowActive | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 12462 | MeleeWindowActive | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12463 | MeleeWindowActive | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12470 | RefreshArmsAnimActive | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12589 | FgRememberArms | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12590 | FgRememberArms | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12655 | LatchFgMeshes | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12656 | LatchFgMeshes | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12657 | LatchFgMeshes | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12659 | LatchFgMeshes | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12660 | LatchFgMeshes | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12684 | ApplyAimMethod | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12690 | ApplyAimMethod | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12709 | ApplyAimMethod | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12710 | ApplyAimMethod | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12712 | ApplyAimMethod | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12718 | ApplyAimMethod | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12751 | FgLearnSignature | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12752 | FgLearnSignature | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12830 | GunSigns | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12925 | GunAimDir | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12927 | GunAimDir | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12956 | GunPivotWorld | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12957 | GunPivotWorld | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12958 | GunPivotWorld | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 12978 | GunPivotWorld | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13083 | BuildGunC | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13087 | BuildGunC | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13088 | BuildGunC | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13089 | BuildGunC | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13104 | PrepareIndependentLeft | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 13136 | PrepareIndependentLeft | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 13149 | PrepareIndependentLeft | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 13176 | FgIsMesh | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13177 | FgIsMesh | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13209 | FgMatchWindow | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| d3d9.cpp | 13212 | FgMatchWindow | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13310 | FgSoloStep | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13344 | FgSoloStep | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13345 | FgSoloStep | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13346 | FgSoloStep | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13551 | AdsCapture | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13556 | AdsCapture | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13568 | AdsMask | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13570 | AdsMask | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13585 | AdsMask | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13594 | AdsFreeze | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13606 | AdsFreeze | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13707 | FgRidesGun | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13708 | FgRidesGun | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13723 | FgRidesGun | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13736 | ReportFgRide | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13740 | ReportFgRide | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13748 | ReportFgRide | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13751 | ReportFgRide | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 13752 | ReportFgRide | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13773 | ReportFgRide | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13776 | ReportFgRide | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 13806 | ArmsMeshNow | `Now()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 13869 | TmdTriSlotApplies | `Now()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 13898 | TmdTriSegments | `Now()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 13928 | TmdTriSegments | `Now()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 14023 | LogFgList | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14026 | LogFgList | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14039 | CheckFgCoexist | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14042 | CheckFgCoexist | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14053 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14057 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14058 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14059 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14079 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14080 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14081 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14103 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14105 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14106 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14107 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14164 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14174 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14175 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14176 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14195 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14196 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14197 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14229 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14271 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14273 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14275 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14312 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14313 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14325 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14327 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14328 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14329 | UpdateFgLatch | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14332 | UpdateFgLatch | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14350 | DpgRecord | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14351 | DpgRecord | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14354 | DpgRecord | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14377 | Hook_Clear | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14378 | Hook_Clear | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 14383 | DumpDpgProbe | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14384 | DumpDpgProbe | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14407 | DumpDpgProbe | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14411 | DumpDpgProbe | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14414 | DumpDpgProbe | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 14432 | CaptureRenderEvidence | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| d3d9.cpp | 14441 | CaptureRenderEvidence | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| d3d9.cpp | 14506 | TrackFpCaster | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| d3d9.cpp | 14510 | TrackFpCaster | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| d3d9.cpp | 14533 | Hook_DrawIndexedPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14535 | Hook_DrawIndexedPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14535 | Hook_DrawIndexedPrim | `g_boundShaderLayout.shadow` | shadow-classify | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14537 | Hook_DrawIndexedPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14539 | Hook_DrawIndexedPrim | `g_boundShaderLayout.shadow` | shadow-classify | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14550 | Hook_DrawIndexedPrim | `Now()` | timing-qpc | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14551 | Hook_DrawIndexedPrim | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14552 | Hook_DrawIndexedPrim | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14569 | Hook_DrawIndexedPrim | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14587 | Hook_DrawIndexedPrim | `MsSince(` | timing-qpc | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14606 | Hook_DrawIndexedPrim | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14685 | Hook_DrawIndexedPrim | `Now()` | timing-qpc | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 14875 | UserPtrInert | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 15186 | ReportXhairCensus | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 15187 | ReportXhairCensus | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 15188 | ReportXhairCensus | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 15192 | ReportXhairCensus | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| d3d9.cpp | 15253 | DumpUserPtrTransform | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 15372 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15373 | HudStereoPair | `g_boundShaderLayout.shadow` | shadow-classify | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15375 | HudStereoPair | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15396 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15424 | HudStereoPair | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15449 | HudStereoPair | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15475 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15477 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15483 | HudStereoPair | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15484 | HudStereoPair | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15486 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15487 | HudStereoPair | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15495 | HudStereoPair | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15499 | HudStereoPair | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15505 | HudStereoPair | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15524 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15526 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15531 | HudStereoPair | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15535 | HudStereoPair | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15536 | HudStereoPair | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15543 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15544 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15545 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15546 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15547 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15548 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15562 | HudStereoPair | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15572 | HudStereoPair | `->SetScissorRect(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15573 | HudStereoPair | `->SetVertexShaderConstantF(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15579 | HudStereoPair | `->SetVertexShaderConstantF(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15580 | HudStereoPair | `->SetScissorRect(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15581 | HudStereoPair | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15752 | DrawSpriteQuadPerEye | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 15765 | DrawSpriteQuadPerEye | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 15766 | DrawSpriteQuadPerEye | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 15789 | DrawVideoQuadPerEye | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 15804 | DrawVideoQuadPerEye | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 15872 | Hook_DrawPrimUP | `g_boundShaderLayout.shadow` | shadow-classify | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15894 | Hook_DrawPrimUP | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15899 | Hook_DrawPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15905 | Hook_DrawPrimUP | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15931 | Hook_DrawPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15943 | Hook_DrawPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15962 | Hook_DrawPrimUP | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15971 | Hook_DrawPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15981 | Hook_DrawPrimUP | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 15998 | Hook_DrawIndexedPrimUP | `g_boundShaderLayout.shadow` | shadow-classify | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16012 | Hook_DrawIndexedPrimUP | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16017 | Hook_DrawIndexedPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16023 | Hook_DrawIndexedPrimUP | `InterlockedExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16049 | Hook_DrawIndexedPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16061 | Hook_DrawIndexedPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16088 | Hook_DrawIndexedPrimUP | `InterlockedCompareExchange` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16095 | Hook_DrawIndexedPrimUP | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16109 | Hook_DrawIndexedPrimUP | `->SetRenderState(` | device-state | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16110 | Hook_DrawIndexedPrimUP | `InterlockedIncrement` | interlocked-hot | per-draw (x1000s/frame) | LOW |
| d3d9.cpp | 16207 | OverrideOcclusion | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16498 | Hook_TexLockRect | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16499 | Hook_TexLockRect | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16500 | Hook_TexLockRect | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16549 | Hook_TexGetSurfaceLevel | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16650 | ProjectedShadowTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16668 | Hook_SetTexture | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 16669 | Hook_SetTexture | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 16672 | Hook_SetTexture | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 16680 | SuppressCapturedVrMenuDraw | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| d3d9.cpp | 16682 | SuppressCapturedVrMenuDraw | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16683 | SuppressCapturedVrMenuDraw | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16684 | SuppressCapturedVrMenuDraw | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16706 | Hook_CreateTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16712 | Hook_CreateTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16722 | Hook_CreateTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16729 | Hook_CreateTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16741 | Hook_CreateCubeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16742 | Hook_CreateCubeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16751 | Hook_CreateCubeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16754 | Hook_CreateCubeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16766 | Hook_CreateVolumeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16767 | Hook_CreateVolumeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16776 | Hook_CreateVolumeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16779 | Hook_CreateVolumeTexture | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16788 | Hook_CreateVertexBuffer | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16795 | Hook_CreateIndexBuffer | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16803 | PatchTextureVTableOnce | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16815 | PatchCubeVTableOnce | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16824 | PatchVolVTableOnce | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16841 | ReportPoolTranslation | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16842 | ReportPoolTranslation | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16843 | ReportPoolTranslation | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16859 | RefreshCameraPosition | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16861 | RefreshCameraPosition | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16878 | RefreshCameraPosition | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16879 | RefreshCameraPosition | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16880 | RefreshCameraPosition | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16907 | RefreshCameraPosition | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16922 | ApplyVrFov | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 16942 | ApplyVrFov | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17059 | ApplyVrFov | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17083 | ReportScan | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17269 | ScriptFuncNameIdx | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17287 | ScriptFuncNameIdx | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17297 | ScriptFuncNameIdx | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17303 | ScriptFuncNameIdx | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17307 | ScriptFuncNameIdx | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17312 | ScriptFuncNameIdx | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17320 | ScriptFuncNameIdx | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17328 | ScriptFuncNameIdx | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17466 | FnIdx | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17480 | NoteNested | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17482 | NoteNested | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17484 | NoteNested | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17486 | NoteNested | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17523 | LocalsOffset | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17551 | RewriteSetAimParams | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17561 | RewriteSetAimParams | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17562 | RewriteSetAimParams | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17578 | RewriteSetAimParams | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17766 | ProbeTraceNative | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17912 | Hook_LineCheck | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17920 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17922 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17933 | Hook_LineCheck | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17934 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17936 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17954 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17956 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17958 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17983 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17989 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17991 | Hook_LineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 17992 | Hook_LineCheck | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18052 | Hook_SingleLineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18054 | Hook_SingleLineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18055 | Hook_SingleLineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18067 | Hook_SingleLineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18069 | Hook_SingleLineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18075 | Hook_SingleLineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18081 | Hook_SingleLineCheck | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18089 | Hook_SingleLineCheck | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18230 | RepointScriptHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18231 | RepointScriptHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18232 | RepointScriptHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18233 | RepointScriptHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18234 | RepointScriptHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18235 | RepointScriptHook | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18258 | ResolveScriptNames | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18312 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18338 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18413 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18416 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18434 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18439 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18440 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18444 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18478 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18480 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18482 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18515 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18528 | Hook_ProcessEvent | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18529 | Hook_ProcessEvent | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18537 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18538 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18545 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18546 | Hook_ProcessEvent | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18547 | Hook_ProcessEvent | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18555 | Hook_ProcessEvent | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18556 | Hook_ProcessEvent | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18681 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18687 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18692 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18696 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18697 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18727 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18728 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18729 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18762 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18784 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18788 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18794 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18813 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18818 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18819 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18820 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18822 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18868 | Hook_ProcessEvent | `InterlockedDecrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18869 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18874 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18876 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18885 | Hook_ProcessEvent | `InterlockedDecrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18915 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18916 | Hook_ProcessEvent | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18917 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18927 | Hook_ProcessEvent | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18936 | DumpProcessEventCensus | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18937 | DumpProcessEventCensus | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18987 | DumpProcessEventCensus | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18988 | DumpProcessEventCensus | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 18998 | DumpProcessEventCensus | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19059 | WriteAimAtInputPoll | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19060 | WriteAimAtInputPoll | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19063 | WriteAimAtInputPoll | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19064 | WriteAimAtInputPoll | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19146 | SetVrMode | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19147 | SetVrMode | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19148 | SetVrMode | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19149 | SetVrMode | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19152 | SetVrMode | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19339 | AnchorMarkerRects | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19340 | AnchorMarkerRects | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19341 | AnchorMarkerRects | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19376 | TmdMarkerRects | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19377 | TmdMarkerRects | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19378 | TmdMarkerRects | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19451 | DrawVrPanel | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19462 | DrawVrPanel | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19597 | DrawVrPanel | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19602 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19603 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19604 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19651 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19652 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19692 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19693 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19694 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19704 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19714 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19715 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19719 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19729 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19730 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19731 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19733 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19751 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19765 | DrawStateReadout | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19769 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19821 | DrawStateReadout | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19829 | DrawStateReadout | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19878 | DrawLaserSight | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19880 | DrawLaserSight | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19881 | DrawLaserSight | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19898 | DrawLaserSight | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 19918 | DrawLaserSight | `->SetTexture(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19920 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19921 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19922 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19923 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19924 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19965 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19966 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19967 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19968 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 19969 | DrawLaserSight | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 20015 | DrawWristHealth | `->SetTexture(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 20016 | DrawWristHealth | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 20017 | DrawWristHealth | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 20018 | DrawWristHealth | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 20019 | DrawWristHealth | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 20020 | DrawWristHealth | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| d3d9.cpp | 20065 | Hook_Present | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20068 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20069 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20073 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20089 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20120 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20121 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20123 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20124 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20131 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20132 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20138 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20139 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20144 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20145 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20150 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20151 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20165 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20166 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20183 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20184 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20204 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20205 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20213 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20214 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20221 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20222 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20227 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20228 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20276 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20285 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20286 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20297 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20298 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20302 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20311 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20312 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20325 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20327 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20338 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20339 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20352 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20353 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20394 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20398 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20408 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20412 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20413 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20414 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20415 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20421 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20586 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20587 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20588 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20589 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20592 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20610 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20611 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20631 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20632 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20639 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20640 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20641 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20675 | Hook_Present | `GetTickCount64()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20699 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20700 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20704 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20705 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20706 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20707 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20712 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20730 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20763 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20784 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20838 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20846 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20898 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20899 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20943 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20944 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20957 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20958 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20971 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20972 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20977 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20978 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20989 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 20990 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21010 | Hook_Present | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21015 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21023 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21049 | Hook_Present | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21059 | Hook_Present | `QueryPerformanceFrequency` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21060 | Hook_Present | `QueryPerformanceCounter` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21072 | Hook_Present | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21092 | Hook_Present | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21096 | Hook_Present | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21098 | Hook_Present | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21110 | Hook_Present | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21112 | Hook_Present | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21128 | Hook_Present | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21147 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21194 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21246 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21290 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21299 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21318 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21390 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21391 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21400 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21406 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21408 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21419 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21427 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21431 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21442 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21448 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21458 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21459 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21460 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21471 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21485 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21486 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21490 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21558 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21560 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21570 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21654 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21660 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21664 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21672 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21673 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21725 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21902 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21932 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21940 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21941 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21942 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21945 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21954 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21955 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21958 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 21995 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22057 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22064 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22065 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22069 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22073 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22077 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22078 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22089 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22090 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22093 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22094 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22095 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22106 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22112 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22119 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22120 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22122 | Hook_Present | `InterlockedDecrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22124 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22134 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22142 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22146 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22155 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22156 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22158 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22159 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22161 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22165 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22218 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22219 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22220 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22236 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22248 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22260 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22279 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22316 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22318 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22320 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22365 | Hook_Present | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22366 | Hook_Present | `InterlockedIncrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22387 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22437 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22438 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22442 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22478 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22479 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22512 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22561 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22565 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22567 | Hook_Present | `InterlockedDecrement` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22569 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22570 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22571 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22572 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22574 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22578 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22579 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22581 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22585 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22586 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22587 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22588 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22598 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22600 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22601 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22602 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22603 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22618 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22647 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22655 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22659 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22660 | Hook_Present | `InterlockedExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22662 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22672 | Hook_Present | `Now()` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22676 | Hook_Present | `MsSince(` | timing-qpc | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22686 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22687 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22696 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22697 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22698 | Hook_Present | `InterlockedCompareExchange` | interlocked-hot | per-frame (~90-120/s) | LOW |
| d3d9.cpp | 22723 | Hook_ResetDevice | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22724 | Hook_ResetDevice | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22729 | Hook_ResetDeviceEx | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22730 | Hook_ResetDeviceEx | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22908 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22910 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22911 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22918 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22919 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22920 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22931 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22932 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 22933 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23090 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23099 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23121 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23134 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23158 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23188 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23212 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23225 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23249 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23257 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23269 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23273 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23278 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23290 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23353 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23354 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23392 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23402 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23475 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23479 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23492 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23496 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23513 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23554 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23565 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23575 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23580 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23582 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23583 | LoadIniSettings | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23589 | LoadIniSettings | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| d3d9.cpp | 23654 | Hook_CreateDevice | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 35 | DiagUs | `QueryPerformanceCounter` | timing-qpc | helper (verify call path) | LOW |
| diagnostic_trace.h | 36 | DiagUs | `QueryPerformanceFrequency` | timing-qpc | helper (verify call path) | LOW |
| diagnostic_trace.h | 40 | DiagQpc | `QueryPerformanceCounter` | timing-qpc | helper (verify call path) | LOW |
| diagnostic_trace.h | 43 | DiagMax | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 46 | DiagXi | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 48 | DiagXi | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 49 | DiagXi | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 52 | DiagStart | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 56 | DiagStart | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 60 | DiagStart | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 61 | DiagStart | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 62 | DiagStart | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 63 | DiagStart | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 64 | DiagStart | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 65 | DiagStart | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 66 | DiagStart | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 67 | DiagStart | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 70 | DiagPhaseAdd | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 73 | DiagFinish | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 134 | DiagWorker | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 153 | DiagWorker | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 169 | DiagWorker | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 193 | DiagEnable | `QueryPerformanceFrequency` | timing-qpc | helper (verify call path) | LOW |
| diagnostic_trace.h | 197 | DiagEnable | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| diagnostic_trace.h | 200 | DiagDisable | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 65 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 67 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 68 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 69 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 70 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 72 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 73 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 74 | configure | `QueryPerformanceFrequency` | timing-qpc | helper (verify call path) | LOW |
| hotpath_sampler.h | 75 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 76 | configure | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 79 | active | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 81 | frame_token | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 86 | next_xr_frame | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 92 | advance_frame | `InterlockedDecrement` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 93 | advance_frame | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 99 | count | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 102 | counter | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 107 | record_call | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 110 | should_time | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 115 | record_sample | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 116 | record_sample | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 121 | read | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 122 | read | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 123 | read | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| hotpath_sampler.h | 153 | scope | `QueryPerformanceCounter` | timing-qpc | helper (verify call path) | LOW |
| hotpath_sampler.h | 165 | finish | `QueryPerformanceCounter` | timing-qpc | helper (verify call path) | LOW |
| input_classification_cache.h | 16 | Invalidate | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| input_classification_cache.h | 20 | Query | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| input_classification_cache.h | 25 | Query | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| input_classification_cache.h | 30 | Query | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| input_classification_cache.h | 37 | Query | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| input_wmi_cache.h | 67 | ReportInputClassificationCache | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| input_wmi_cache.h | 68 | ReportInputClassificationCache | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| menu_draw_trace.h | 50 | MenuDrawTraceState | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| menu_draw_trace.h | 62 | MenuDrawTraceRecord | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| missing_mouse.h | 75 | Install | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| missing_mouse.h | 86 | Install | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| missing_mouse.h | 90 | Install | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| missing_mouse.h | 99 | Install | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| missing_mouse.h | 101 | Install | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 47 | UpdateMenu | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 48 | UpdateMenu | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 51 | UpdateMenu | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 52 | UpdateMenu | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 57 | Coordinates | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 58 | Coordinates | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 63 | Coordinates | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 95 | BindViewport | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 104 | Drain | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 116 | Drain | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 119 | Drain | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 122 | Drain | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| native_ui_input.h | 132 | Install | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| psvr2_client.h | 176 | Worker | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| psvr2_client.h | 215 | Worker | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| psvr2_client.h | 217 | Worker | `GetTickCount64()` | timing-qpc | helper (verify call path) | LOW |
| startup_crash_capture.h | 9 | CaptureStartupException | `InterlockedCompareExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 46 | DrawVrPromptLegend | `->SetTexture(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 66 | DrawVrPromptLegend | `->SetTexture(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 85 | DrawVrMenuLaser | `->SetTexture(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 87 | DrawVrMenuLaser | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 88 | DrawVrMenuLaser | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 89 | DrawVrMenuLaser | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 107 | UpdateVrUiPointer | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 120 | UpdateVrUiPointer | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 121 | UpdateVrUiPointer | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 122 | UpdateVrUiPointer | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 124 | UpdateVrUiPointer | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 125 | UpdateVrUiPointer | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 130 | SyncVrMenuCaptureState | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 137 | SyncVrMenuCaptureState | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 143 | CaptureVrMenuTarget | `g_boundShaderLayout.shadow` | shadow-classify | helper (verify call path) | LOW |
| vr_menu_overlay.h | 163 | CaptureVrMenuTarget | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 164 | CaptureVrMenuTarget | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 165 | CaptureVrMenuTarget | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 191 | ReleaseVrMenuResources | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 213 | DrawVrMenuPointer | `->SetViewport(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 215 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 216 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 217 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 218 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 219 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 220 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 221 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 222 | DrawVrMenuPointer | `->SetRenderState(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 244 | DrawVrMenuPointer | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 246 | DrawVrMenuPointer | `->SetTexture(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 249 | DrawVrMenuPointer | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 250 | DrawVrMenuPointer | `InterlockedIncrement` | interlocked-hot | helper (verify call path) | LOW |
| vr_menu_overlay.h | 252 | DrawVrMenuPointer | `->SetTexture(` | device-state | helper (verify call path) | LOW |
| vr_menu_overlay.h | 253 | DrawVrMenuPointer | `InterlockedExchange` | interlocked-hot | helper (verify call path) | LOW |

## Rationale per category

### com-get-state

D3D9 Get* call on the device: vtable hop plus driver validation, and on some drivers it stalls the pipeline. Per draw it also AddRefs the returned surface, doubling the cost with the matching Release.

### com-get-resource

COM getter: cheap in isolation, but each call is an interlocked AddRef on the returned object plus validation. Per draw x2 eyes it adds up; cache the pointer instead of re-querying.

### com-getdesc

GetDesc copies a descriptor struct through a COM call. Harmless once, wasteful per draw; hoist the size/format out of the hot path.

### com-get-constants

Reading shader constants back from the device per draw forces a CPU-side shadow copy lookup at best and a driver round-trip at worst. The shadow-pass dump in StereoPairBody is correctly one-shot gated; any UNGATED occurrence is a bug.

### com-refcount

Interlocked refcount: takes the cache line exclusively. One pair per draw at thousands of draws/frame is cross-thread line bouncing against the render thread. Usually paired with a com-get above - fix the getter and this goes away too.

### shader-ctab

Shader reflection parses bytecode tables. Milliseconds, not microseconds - per draw it would dominate the frame. Must run at shader-bind or load time only.

### lock

Blocking lock on a render path risks priority inversion and serialises the two eye draws. Prefer the file's Rd()/plain-word pattern for reads; keep interlocked ops for writes only.

### interlocked-hot

A single interlocked op is ~10-20ns, NOT a bug by itself. It matters only in aggregate: per draw x2 eyes x thousands of draws against a word Present also writes = cache-line fight (see the run-203 note on Rd()). Listed so the aggregate can be eyeballed, not as a defect.

### heap-alloc

Heap allocation takes the process heap lock and can page. Per draw it fragments and serialises; use stack/fixed storage (the hotpath_sampler pattern) instead. (`= delete` declarations are excluded.)

### string-alloc

std::string construction typically heap-allocates. Per draw this is a hidden malloc per call; prefer fixed char buffers (the file already does this for log lines).

### file-io

File I/O is milliseconds (open/write/close syscalls). The run-202 incident (120 fps -> 11 fps from a per-frame Log) proves this class. Log() here is async-batched, but the enqueue still costs per draw.

### log-call

Log() formats AND enqueues under contention. Rate-limited/one-shot calls are fine; any Log reachable per draw is a repeat of run-202. Verify the call site is gated before dismissing.

### timing-qpc

QPC is ~20-50ns - free for per-frame use, and the file deliberately times StereoPair with it. Listed only because per-DRAW pairs (start+end x4000 draws) reach ~0.5ms/frame; keep at most one pair per draw.

### virtualquery

VirtualQuery is a kernel transition (~microseconds). The run-35/256 incidents (hundreds of thousands of syscalls freezing menus) are this class. Must never run per draw; the 8-slot region cache only bounds walks, it does not make them hot-path safe.

### object-walk

GObjects walks are tens of thousands of objects with syscalls per object. The file already gates these behind backoffs and once-a-frame resolves; any NEW call from a draw hook reintroduces run-35.

### openxr-call

OpenXR runtime calls cross into the runtime and may block (WaitFrame) or cost microseconds (locate/sync). Correct per FRAME from UpdateFromHeadset; HIGH severity if found inside a per-DRAW hook - check the containing function.

### xr-submit

Submission/copy path: the measured black box (run-56). Timing it is good; duplicating the copy or reading back the target per draw is not.

### ik-solve

IK/trig per draw is unusual - normally once per frame per limb. atan2/acos are tens of ns each; fine per frame, suspect per draw. Verify call frequency before acting.

### menu-classify

Menu/UI classification runs per user-pointer draw. String compares or texture queries here multiply by the UI draw count; keep to integer tag compares on the hot path.

### shadow-classify

Shadow classification itself is usually a flag check (cheap). Flagged so reviewers can confirm no surface/texture queries hide behind it per draw.

### sleep

Sleep on a render/input thread surrenders the quantum (ms). The file hooks Sleep precisely because UE3's smoother sleeps the frame; any ADDITIONAL sleep on this path compounds it.

### device-state

Device state uploads are the MECHANISM of StereoPair (per-eye re-upload is intended). Listed only for completeness: an EXTRA redundant upload per draw (same values twice) is the only actionable sub-case.

