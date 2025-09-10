# Services

## 🟦 Handler (e.g. `LineHandler`)
**Responsibility:**  
A *Handler* represents a **single entity** (e.g., a telephone line).  
It stores all the state and variables for that entity and provides methods to update them.

**Example tasks of a `LineHandler`:**
- Keep track of the line’s number and phone number.  
- Store current and previous status (idle, busy, ringing, connected, …).  
- Handle signals like hook status, pulsing, and dialed digits.  
- Start and stop line-specific timers.  

The Handler is essentially a **model of one line**.  

---

## 🟩 Manager (e.g. `LineManager`)
**Responsibility:**  
A *Manager* is responsible for handling **multiple Handlers** at the same time.  
It acts as a container that creates, owns, and updates all the lines in the system.

**Example tasks of a `LineManager`:**
- Create all `LineHandler` objects based on the number of active lines.  
- Iterate through the lines and update their status.  
- Provide access to a specific line (`getLine(index)`).  
- Forward changes to `LineAction` when a line’s status is updated.  

The Manager works as the **coordinator** for all lines.  

---

## 🟥 Action (e.g. `LineAction`)
**Responsibility:**  
An *Action* contains the logic for **what happens when something changes**.  
It acts on one (or several) lines whenever their status is updated.

**Example tasks of a `LineAction`:**
- Execute actions when a line changes status (e.g., starts ringing).  
- Process incoming and outgoing calls.  
- Handle dialed digits (tone or pulse dialing).  
- Connect two lines during an active call.  

The Action is essentially the **brain** that decides the behavior based on line states.  

---
- `LineHandler` → stores data and status for **one line**.  
- `LineManager` → manages **all lines** and monitors state changes.  
- `LineAction` → executes **logic** when something should happen.  

### Flow:
1. A line (`LineHandler`) changes status → e.g., from *Idle* to *Ringing*.  
2. `LineManager` detects the change in its update loop.  
3. `LineManager` forwards the line to `LineAction`.  
4. `LineAction` executes the corresponding logic (e.g., play ringing tone, connect to another line).

---

## 🟨 SHK (Hook) Service (e.g. `SHKService`)
**Responsibility:**  
Monitors the physical hook (on-hook / off-hook) for each line, debounces raw signals, and triggers state transitions (e.g. `idle → ready`). It tells the rest of the system which line just became active or returned to rest.

**What it does:**
- Samples hook inputs (GPIO / expander).
- Debounces noisy mechanical changes.
- Keeps a lightweight hook state per line.
- Notifies when a line goes off-hook (start dial tone logic) or on-hook (call teardown).
- Provides the “most recently lifted” line reference for dialing (pulse or DTMF).

**Typical trigger chain:**
1. Handset lifted → stable off-hook detected.  
2. `SHKService` marks line as active.  
3. Manager / Action logic moves line to `ready` and starts dial tone / timers.  
4. Handset returned → `SHKService` reports on-hook → Action logic ends or resets call state.  

**Provides (examples):**
- `isOffHook(line)`  
- `wasJustLifted(line)`  
- `wasJustHungUp(line)`  

Keeps the hook layer clean so higher-level logic only reacts to stable, meaningful changes.
