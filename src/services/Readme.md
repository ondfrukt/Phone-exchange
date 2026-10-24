# Services

> Not exhaustive — missing e.g. `RingGenerator`, `ToneGenerator`, `ConnectionHandler`, `AudioPlayer`. See `docs/CODEMAP.md` when #102 is done.

## 🟦 LineHandler
**Responsibility:**  
Represents ONE physical/logical phone line. Holds all mutable state for that line (status, hook, timers, dialing progress, call endpoints).

**What it tracks:**
- Identity: `lineNumber`, `lineActive`, `phoneNumber`
- Status history: `currentLineStatus`, `previousLineStatus`
- Call endpoints: `incomingFrom`, `outgoingTo`
- Hook state: `currentHookStatus`, `previousHookStatus`, raw `SHK`
- Dialing (pulse / DTMF): `dialedDigits`, pulse timing (`gap`, `edge`)
- Line timer: `lineTimerStart`, `lineTimerLimit`, `lineTimerActive`

**Key methods:**
- `LineHandler(int line)` – initializes all fields
- `startLineTimer(limit)` / `stopLineTimer()` – manage per-line timeout logic
- `lineIdle()` – reset transient call / dialing fields when returning to Idle

**Used by:** `LineManager` (owns and updates instances), Action logic (reacts to status transitions).

---

## 🟩 LineManager
**Responsibility:**  
Owns all `LineHandler` objects (currently 0–7). Provides safe access, updates line statuses, and emits callbacks when a line changes state.

**What it does:**
- Creates 8 handlers in constructor (sets `lineActive` from `Settings.activeLinesMask`)
- Initializes lines via `begin()`
- Changes status with `setStatus(index, newStatus)` (updates previous, triggers reset on Idle, sets bit in `lineChangeFlag`)
- Exposes a per-line timer helper: `setLineTimer(index, limit)` (indirectly via handler)
- Notifies observers: `setStatusChangedCallback(cb)`
- Tracks changed lines: `lineChangeFlag` bitmask
- Maintains active line timers bitmask: `activeTimersMask`

**Key API:**
```cpp
LineHandler& getLine(int index);
void setStatus(int index, LineStatus newStatus);
void clearChangeFlag(int index);
void setStatusChangedCallback(StatusChangedCallback cb);
void syncLineActive(size_t i); // Re-sync active flag from settings
```

**Callbacks:**
- `StatusChangedCallback(int lineIndex, LineStatus newStatus)` – fired after every status change

**Debug behavior:**  
Conditional `Serial` logging based on `settings_.debugLmLevel`.

**Error handling:**  
Out-of-range index → logs and returns first line (in `getLine`) or early return (in mutators), avoiding exceptions in Arduino context.

---

## 🟥 Action (e.g. `LineAction`)

Reacts to hook/status changes and timer expiry, and drives tones, ringing, the connection matrix and MQTT. See `LineAction.cpp` (`update`, `action`, `timerExpired`).

---

## 🟨 SHK (Hook) Service (e.g. `SHKService`)
**Responsibility:**  
Monitors the physical hook (on-hook / off-hook) for each line, debounces raw signals, and triggers state transitions (e.g. `idle → ready`).

**What it does:**
- Samples hook inputs (GPIO / expander).
- Debounces noisy mechanical changes.
- Keeps a lightweight hook state per line.
- Notifies when a line goes off-hook
- Provides the “most recently lifted” line reference for dialing (pulse or DTMF).

**Typical trigger chain:**
1. Handset lifted → stable off-hook detected.  
2. `SHKService` marks line as active.  
3. LineManager logic moves line to `ready` and LineAction starts dial tone / timers.
4. Handset returned → `SHKService` reports on-hook → LineManager change the state and LineAction logic ends or resets call state.  

Keeps the hook layer clean so higher-level logic only reacts to stable, meaningful changes.

---

## 🟪 ToneReader
**Responsibility:**  
Monitors the MT8870 DTMF decoder chip and converts detected tones into digits for the active phone line.

**What it does:**
- Detects rising/falling edges on the MT8870 STD (Steering Data) signal via interrupts
- Reads the 4-bit DTMF code (Q1-Q4) when a valid tone is detected
- Implements configurable filtering to reject spurious signals and noise
- Associates detected tones with the most recently active line (`lastLineReady`)
- Only accepts tones when the line is in `Ready` or `ToneDialing` state

**Key filtering mechanisms:**
1. **STD Signal Stability** (`dtmfStdStableMs`): Waits for the STD signal to remain high for a minimum duration before reading the tone, filtering very short glitches
2. **Minimum Tone Duration** (`dtmfMinToneDurationMs`): Rejects tones that are shorter than the minimum valid DTMF duration (MT8870 spec: 40ms typical)
3. **Debounce Time** (`dtmfDebounceMs`): Prevents duplicate detection of the same digit within a configurable time window

**Configurable settings (in Settings class):**
- `dtmfDebounceMs` - Time between same digit detections
- `dtmfMinToneDurationMs` - Minimum valid tone duration
- `dtmfStdStableMs` - STD signal stability requirement
- `debugTRLevel` (0-2) - Debug verbosity level

Default values: see `Settings.cpp`.

**Typical flow:**
1. Phone goes off-hook → line status becomes `Ready`
2. User presses a digit → MT8870 decodes the DTMF tones
3. STD signal goes HIGH → ToneReader marks rising edge as pending
4. After `dtmfStdStableMs`, STD is still HIGH → read Q1-Q4 pins
5. Decode nibble to character (0-9, *, #)
6. Check debounce filter (not same digit within `dtmfDebounceMs`)
7. Append digit to line's `dialedDigits` string
8. STD goes LOW → measure duration, reject if < `dtmfMinToneDurationMs`
9. Set line status to `ToneDialing` and start dial timer

**Error handling:**
- Rejects tones when no valid `lastLineReady` line exists
- Filters out invalid nibbles that don't map to valid DTMF characters
- Only accepts tones when line is in appropriate state (Ready/ToneDialing)

**Debug levels:**
- Level 0: No debug output
- Level 1: Basic events (edges, accepted/rejected tones, warnings)
- Level 2: Detailed debugging (STD signal details, debounce checks, raw GPIO values)
