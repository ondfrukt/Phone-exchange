# ToneReader Configuration Guide

## Overview
The ToneReader class has been improved to better filter out spurious signals and noise that were being incorrectly detected as DTMF tones. Three new configurable parameters have been added to the Settings system to allow fine-tuning of the tone detection behavior.

## New Configuration Parameters

### 1. `dtmfDebounceMs` (Default: 150ms)
**Purpose:** Prevents duplicate detection of the same digit within a short time period.

**How it works:** After a digit is successfully detected and processed, the same digit will be ignored if detected again within this time window. This prevents a single button press from being registered multiple times.

**When to adjust:**
- **Increase** if you're getting duplicate digits (e.g., pressing "5" once but getting "55")
- **Decrease** if legitimate rapid same-digit sequences are being missed (e.g., "55" in a phone number)

**Recommended range:** 100-300ms

---

### 2. `dtmfMinToneDurationMs` (Default: 40ms)
**Purpose:** Filters out very short electrical noise or glitches that trigger the MT8870 but aren't valid DTMF tones.

**How it works:** The system measures how long the STD (Steering Data) signal from the MT8870 remains high. If it drops low before reaching this minimum duration, the tone is rejected as invalid.

**When to adjust:**
- **Increase** if you're getting random digit detections from electrical noise or interference
- **Decrease** if legitimate very quick button presses aren't being detected (unusual - most phones generate 40-100ms tones minimum)

**Recommended range:** 30-60ms

**Note:** The MT8870 datasheet specifies 40ms as the typical minimum tone duration for reliable DTMF detection, so values below 30ms are not recommended.

---

### 3. `dtmfStdStableMs` (Default: 5ms)
**Purpose:** Ensures the STD signal has stabilized before reading the DTMF code from the Q1-Q4 pins.

**How it works:** When the STD signal transitions from LOW to HIGH (rising edge), the system waits this duration before reading the 4-bit code. This prevents reading unstable/transitioning values during signal rise time.

**When to adjust:**
- **Increase** if you're getting incorrect digits detected (e.g., pressing "5" but getting "8" or other wrong digits)
- **Decrease** if legitimate tones are being missed (unlikely with default value)

**Recommended range:** 3-10ms

**Note:** This is typically the smallest value you'll need to adjust. The default of 5ms provides good filtering for most scenarios.

---

## How the Filtering Works Together

The ToneReader now implements a three-stage filtering process:

### Stage 1: Rising Edge Detection
When the STD signal goes HIGH, the system marks it as a "pending" tone but doesn't immediately process it.

### Stage 2: Stability Check (`dtmfStdStableMs`)
After the configured stability period, if STD is still HIGH, the system reads the Q1-Q4 pins and decodes the DTMF digit.

### Stage 3: Duration & Debounce Validation
- When STD goes LOW, the total tone duration is calculated
- If duration < `dtmfMinToneDurationMs`, the tone is rejected
- If the same digit was detected < `dtmfDebounceMs` ago, it's rejected as a duplicate
- Otherwise, the digit is accepted and added to the dialed digits

## Troubleshooting Common Issues

### Problem: Getting random digits when no phone is being used
**Solution:** Increase `dtmfMinToneDurationMs` to 50-60ms to better filter electrical noise.

### Problem: Pressing a digit once results in multiple detections
**Solution:** Increase `dtmfDebounceMs` to 200-250ms.

### Problem: Occasionally getting wrong digits (e.g., pressing 5 gives 7)
**Solution:** Increase `dtmfStdStableMs` to 7-10ms to allow more time for the MT8870 output to stabilize.

### Problem: Fast dialing doesn't register all digits
**Solution:** Decrease `dtmfDebounceMs` to 100-120ms (but not too low or you'll get duplicates).

### Problem: Very quick button taps aren't registered
**Solution:** Slightly decrease `dtmfMinToneDurationMs` to 35ms (but not below 30ms).

## Configuring via Web Interface

The new settings can be adjusted through the phone exchange web interface:
1. Navigate to http://phoneexchange.local/
2. Go to Settings section
3. Look for "DTMF/ToneReader Settings"
4. Adjust the values as needed
5. Save changes

## Configuring Programmatically

The settings are accessible via the Settings singleton:

```cpp
Settings& settings = Settings::instance();

// Adjust DTMF settings
settings.dtmfDebounceMs = 200;        // 200ms debounce
settings.dtmfMinToneDurationMs = 45;  // 45ms minimum tone
settings.dtmfStdStableMs = 7;         // 7ms stability check

// Save to non-volatile storage
settings.save();
```

## Debug Logging

To see detailed information about tone detection and filtering, increase the ToneReader debug level:

```cpp
settings.debugTRLevel = 2;  // Maximum verbosity
```

Debug levels:
- **0**: No debug output
- **1**: Basic events (edges, accepted/rejected tones, warnings)
- **2**: Detailed debugging (STD signal details, debounce checks, raw GPIO values)

This will show in the serial console:
- When edges are detected
- STD stability measurements
- Tone duration calculations
- Debounce filter decisions
- Which tones are accepted vs. rejected

## Technical Background

The MT8870 is a DTMF decoder chip that:
- Receives audio signals from phone lines
- Detects DTMF (touch-tone) frequencies
- Outputs a 4-bit code on Q1-Q4 pins representing the digit
- Raises the STD pin HIGH when a valid tone is detected

The improvements implement proper signal conditioning and validation to ensure only genuine DTMF tones from phone keypads are registered, while rejecting electrical noise, crosstalk, and other spurious signals that might trigger the MT8870.
