# Testing Ring Generator Fix

## Problem
When testing ring signal generation from the UI, the ring signal would immediately stop because the FR pin toggling caused electrical interference on the SHK pin, making it appear as if the phone went off-hook.

### Symptoms (Before Fix)
When ringing line 0, the serial output showed:
```
[RingGenerator] Started ringing for line 0 (RM pin 6 on MCP 0x26)
toggling FR pin to 1 on Line 0
toggling FR pin to 0 on Line 0
toggling FR pin to 1 on Line 0
InterruptManager: Queued MCP_SLIC1 event - addr=0x26 pin=5 line=0 level=HIGH
InterruptManager: Queued MCP_SLIC1 event - addr=0x26 pin=5 line=0 level=LOW
...
LineManager: Line 0 status changed to Ready
[RingGenerator] Line 0 status changed from Idle, stopping ring
[RingGenerator] Stopped ringing for line 0
...
LineManager: Line 0 status changed to Idle
```

The line status was incorrectly changing from Idle → Ready → Idle during ringing.

## Solution
The fix prevents SHKService from processing SHK pin interrupts for lines that are currently ringing. This eliminates false off-hook detections caused by electrical interference from FR pin toggling.

### Changes Made
1. **RingGenerator.h/cpp**: Added `isLineRinging()` method to query if a line is currently generating a ring signal
2. **SHKService.h/cpp**: 
   - Added optional reference to RingGenerator
   - Modified `notifyLinesPossiblyChanged()` to filter out interrupts for ringing lines
3. **App.cpp**: Set RingGenerator reference in SHKService during initialization

## How to Test

### Expected Behavior (After Fix)
1. Navigate to the UI and trigger a ring test for line 0
2. Monitor serial output with debug level >= 2
3. Expected output should show:
   ```
   [RingGenerator] Started ringing for line 0 (RM pin 6 on MCP 0x26)
   toggling FR pin to 1 on Line 0
   toggling FR pin to 0 on Line 0
   toggling FR pin to 1 on Line 0
   toggling FR pin to 0 on Line 0
   ...
   [RingGenerator] Line 0 ring iteration X complete, pausing for Y ms
   [RingGenerator] Line 0 starting ring iteration X
   ...
   [RingGenerator] Line 0 completed all N ring iterations
   [RingGenerator] Stopped ringing for line 0
   ```

4. **Important**: The line status should remain **Idle** throughout the entire ring sequence
5. There should be **NO** messages about "Line 0 status changed to Ready" during ringing
6. SHK interrupts for line 0 should be filtered out during ring generation

### Additional Tests
1. **Test multiple lines**: Verify that ringing works correctly on all lines (0-7)
2. **Test actual phone pickup**: While ringing, pick up the phone - the ring should stop and line should go to Ready
3. **Test other lines during ring**: While one line is ringing, verify that hook detection still works on other non-ringing lines

### Debug Levels
To see detailed debug output, set:
- `debugRGLevel >= 1`: See ring start/stop messages
- `debugRGLevel >= 2`: See FR pin toggling messages
- `debugIMLevel >= 2`: See interrupt queue messages
- `debugLmLevel >= 1`: See line status change messages

## Technical Details

### Root Cause
- FR pin (e.g., pin 7 on MCP 0x26 for line 0) toggles at 20 Hz during ring generation
- SHK pin (e.g., pin 5 on MCP 0x26 for line 0) is on the same MCP chip
- The rapid FR pin toggling causes electrical interference/crosstalk on the SHK pin
- This interference triggers interrupts that are interpreted as hook state changes
- SHKService would process these false interrupts and incorrectly detect the phone as off-hook
- When the line status changed from Idle to Ready, RingGenerator would stop ringing

### Fix Details
The fix adds a filtering mechanism in `SHKService::notifyLinesPossiblyChanged()`:
```cpp
// Filter out lines that are currently ringing to avoid false hook detections
// caused by electrical interference from FR pin toggling
if (ringGenerator_ != nullptr) {
  for (std::size_t i = 0; i < maxPhysicalLines_; ++i) {
    if (ringGenerator_->isLineRinging(i)) {
      changedMask &= ~(1u << i);
    }
  }
}
```

This ensures that while a line is actively ringing:
1. Interrupts on its SHK pin are collected but ignored
2. Hook state detection is suspended for that specific line
3. Other lines continue to function normally
4. When ringing stops, normal hook detection resumes
