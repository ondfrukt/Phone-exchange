# Services

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
- Maintains active line timers bitmask: `activeLineTimers` (planned/used for timers)

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

In work

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

## 🟪 RingGenerator
**Responsibility:**  
Generates ring signals for phone lines by controlling the RM (Ring Mode) and FR (Frequency Ring) pins on the SLIC modules.

**What it does:**
- Generates non-blocking ring signals for specified lines
- Controls RM pin (HIGH to activate ring mode, LOW to deactivate)
- Toggles FR pin at 20 Hz frequency during ring phase to generate proper ring signal
- Manages ring timing: signal duration, pause duration, and number of iterations
- Supports independent ringing on multiple lines simultaneously

**Ring signal pattern:**
Each ring cycle consists of:
1. **Ring phase**: RM pin HIGH, FR pin toggles at 20 Hz (25ms high, 25ms low) for `ringLengthMs` duration
2. **Pause phase**: RM pin HIGH, FR pin LOW for `ringPauseMs` duration
3. Repeats for `ringIterations` times, then stops automatically

**Configuration:**
Settings are read from `Settings` class:
- `ringLengthMs` – Duration of the ring signal (default: 500ms)
- `ringPauseMs` – Duration of the pause between rings (default: 2000ms)
- `ringIterations` – Number of ring cycles (default: 2)

**Key API:**
```cpp
void generateRingSignal(uint8_t lineIndex);  // Start ringing on specified line (0-7)
void stopRinging(uint8_t lineIndex);         // Stop ringing on specified line
void update();                               // Non-blocking update - call regularly from main loop
```

**Debug behavior:**  
Conditional `Serial` logging based on `settings_.debugRGLevel`:
- Level 1: Start/stop events and errors
- Level 2: Phase transitions and pin state changes

**Implementation details:**
- Uses non-blocking timing with `millis()` to avoid delays
- Maintains independent state for each of the 8 lines
- 20 Hz frequency = 50ms period (25ms HIGH + 25ms LOW)
- Automatically stops after completing all iterations
