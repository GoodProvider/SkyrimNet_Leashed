With SkyrimNet_Leashed 0.4.1 enabled, I get two problems that go away completely when only this mod is disabled:

1. **Alt-tab freeze.** After tabbing back into the game, it stays frozen for 12–25 s. During that time the main thread runs at 100% of one core, about 97% of it inside `SkyrimNet_Leashed.dll`.
2. **Regular hitch.** In normal play, every 750 ms (±20 ms), a burst of about 30 ms in `SkyrimNet_Leashed.dll` on an engine thread makes the main thread wait. The result is a 55–70 ms frame at a steady rhythm.

Disabling SkyrimNet Leashed and changing nothing else fixed both: alt-tab is instant, and the 750 ms rhythm is gone.

### Frame timing (PresentMon, 60 s, same spot in the Solitude market)
Yes the FPS is horrific. That is what I was trying to fix, finding the Leashed issue was more or less accidental.

| | Leashed on | Leashed off |
|---|---|---|
| Average FPS | 39.0 | 41.0 |
| 1% low | 15.6 FPS | 20.3 FPS |
| p99 frame time | 58.9 ms | 40.8 ms |
| Frames > 50 ms | 68 | 8 |
| Gaps between hitches of 0.70–0.80 s | 58 | 1 |

### CPU profile (WPR, 1 kHz sampling with stacks, analysed with TraceEvent)

**Alt-tab freeze.** The window is 3.0–24.5 s of the trace, right after focus returned. The main thread has 21,383 samples in the DLL, which is 99.5% of one core:
```
ntdll -> kernel32 -> SkyrimSE -> hdtsmp64 -> SkyrimSE -> skse64_1_6_1170 -> SkyrimNet_Leashed   (79%)
                                            hed -> SkyrimSE          (7%)
```
While the game was unfocused, the DLL used nappens after focus comes back.

**750 ms hitch.** The window is 26–75 s of nad across the engine's worker threads:
```
ntdll -> kernel32 -> SkyrimSE -> skse64_1_6_(~79% on each thread)
```
A periodicity search over every module in thstrictly periodic component:`SkyrimNet_Leashed` at 750.5 ms, with modulation depth 0.995 (1.0 means all of its samples fall at the same point in
every cycle). The main thread's waits line u

**Hot code.** Both cases spend their time in relative to the DLL's image base:

| RVA | Freeze (main thread) | Periodic (wor
|---|---|---|
| `+0x36AA0` | 60.8% | 59–68% |
| `+0x1720` | 8.5% | 7–10% |
| `+0x36AC0` | 7.2% | 5–9% |
| `+0x36A90` | 6.2% | 2–8% |
| `+0x23C90` | 3.0% | 1–4% |
| `+0x628A0` | 1.8% | 1–3% |

`0x36A90`–`0x36AE0` is a tight loop that takes about 75% of the samples, so it looks like a linear scan or search that runs far more often than intended.


Build it was recorded with, so the offsets can be resolved against your PDB:
- `SkyrimNet_Leashed.dll` 0.4.1 release (the file version reads 0.4.0.0), size 743,424 bytes
- PDB `SkyrimNet_Leashed.pdb`, GUID `431e9fe0-7367-4936-9f8c-941998ba626d`, age 6

### Environment
- Skyrim SE 1.6.1170, SKSE 2.2.6 (`skse64_1_6_1170`)
- SkyrimNet beta25 **rc10**. The alt-tab freeze started when I updated from rc8 to rc9 (I think).
- LeashFramework 1.1.3
- Papyrus Tweaks NG, Community Shaders 1.8.4, SSE Engine Fixes
- Windows 11 in a KVM VM, 6 cores / 12 threads (Ryzen 7 9800X3D host)

I can share the ETL trace (about 1.5 GB) if that helps.