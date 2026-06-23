/* Copyright 2024
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#ifndef APP_CW_H
#define APP_CW_H

#ifdef ENABLE_CW

#include <stdbool.h>
#include <stdint.h>

// Maximum keyed message length (excluding the NUL terminator). Must fit the
// shared menu edit buffer (17 bytes) and the reserved EEPROM block.
#define CW_MESSAGE_MAX 16

// Default beacon message used the first time, before anything is stored.
#ifndef CW_BEACON_MESSAGE
#define CW_BEACON_MESSAGE "CQ DE N0CALL"
#endif

// Default keying speed in words-per-minute (PARIS standard).
#ifndef CW_DEFAULT_WPM
#define CW_DEFAULT_WPM 15
#endif

#define CW_WPM_MIN 5
#define CW_WPM_MAX 40

// Selectable MCW side-tone frequencies (Hz). The menu stores an index here.
extern const uint16_t CW_TONE_PRESETS[];
extern const uint8_t CW_TONE_PRESETS_COUNT;
#define CW_DEFAULT_TONE_INDEX 3 // 700 Hz

// Current keying parameters (mirror of what is stored in EEPROM).
extern uint8_t gCwWpm;            // words per minute
extern uint8_t gCwToneIndex;      // index into CW_TONE_PRESETS[]
extern uint16_t gCwToneFrequency; // derived side-tone frequency, Hz

// Load the keying parameters and beacon message from EEPROM. Call once at boot.
void CW_LoadConfig(void);

// Persist the current keying parameters and beacon message to EEPROM.
void CW_SaveConfig(void);

// Setters that update the live value and persist it.
void CW_SetWpm(uint8_t wpm);
void CW_SetToneIndex(uint8_t index);

// Copy the stored beacon message (NUL terminated) into a >= CW_MESSAGE_MAX + 1
// byte buffer.
void CW_GetMessage(char *out);
// Replace the stored beacon message and persist it.
void CW_SetMessage(const char *message);

// Begin keying the given message. Non-blocking: brings the transmitter up,
// then the keying is advanced from CW_TimeSlice10ms(). Beeps and does nothing
// if TX is not currently allowed or a beacon is already running.
void CW_Start(const char *message);

// Begin keying the stored beacon message (side-key action entry point).
void CW_StartBeacon(void);

// Advance the keying state machine. Call every 10 ms.
void CW_TimeSlice10ms(void);

// Stop keying immediately and return the radio to receive.
void CW_Abort(void);

// True while a beacon is being keyed.
bool CW_IsActive(void);

#endif // ENABLE_CW

#endif // APP_CW_H
