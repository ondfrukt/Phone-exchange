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