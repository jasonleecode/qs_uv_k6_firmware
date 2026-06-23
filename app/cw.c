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

#ifdef ENABLE_CW

#include "app/cw.h"

#include <stddef.h>
#include <string.h>

#include "app/battery.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/bk4819.h"
#include "driver/eeprom.h"
#include "driver/gpio.h"
#include "driver/keyboard.h"
#include "misc/audio.h"
#include "misc/frequencies.h"
#include "misc/functions.h"
#include "misc/misc.h"
#include "misc/radio.h"
#include "misc/settings.h"

// EEPROM storage (24 free bytes between the MR channel attributes and the
// FM channel area).
#define CW_EEPROM_CONFIG 0x0E28 // 8 bytes: [0]=wpm, [1]=tone index
#define CW_EEPROM_MESSAGE 0x0E30 // 16 bytes: ASCII beacon text

// Tone tuning gain used while keying (0 ~ 127).
#define CW_TONE_LEVEL 96

// Ignore an abort request for this many ticks after starting, so that the
// key/PTT that launched the beacon does not instantly stop it.
#define CW_START_GUARD_TICKS 12 // 120 ms

const uint16_t CW_TONE_PRESETS[] = {400, 500, 600, 700, 800, 900, 1000, 1200};
const uint8_t CW_TONE_PRESETS_COUNT = ARRAY_SIZE(CW_TONE_PRESETS);

uint8_t gCwWpm = CW_DEFAULT_WPM;
uint8_t gCwToneIndex = CW_DEFAULT_TONE_INDEX;
uint16_t gCwToneFrequency = 700;

static char gCwMessage[CW_MESSAGE_MAX + 1] = CW_BEACON_MESSAGE;

// ---- Morse code table ----------------------------------------------------
// '.' = dit, '-' = dah.
static const char *const cw_letters[26] = {
    ".-",   "-...", "-.-.", "-..",  ".",    "..-.", "--.",  "....", "..",
    ".---", "-.-",  ".-..", "--",   "-.",   "---",  ".--.", "--.-", ".-.",
    "...",  "-",    "..-",  "...-", ".--",  "-..-", "-.--", "--.."};

static const char *const cw_digits[10] = {
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----."};

static const char *CW_Lookup(char c) {
  if (c >= 'a' && c <= 'z') c -= 'a' - 'A';

  if (c >= 'A' && c <= 'Z') return cw_letters[c - 'A'];
  if (c >= '0' && c <= '9') return cw_digits[c - '0'];

  switch (c) {
    case '.': return ".-.-.-";
    case ',': return "--..--";
    case '?': return "..--..";
    case '/': return "-..-.";
    case '=': return "-...-";  // BT / break
    case '+': return ".-.-.";  // AR / end of message
    case '-': return "-....-";
    case '(': return "-.--.";
    case ')': return "-.--.-";
    case ':': return "---...";
    case '\'': return ".----.";
    case '@': return ".--.-.";
    default: return NULL;
  }
}

// ---- Persisted configuration --------------------------------------------
static void CW_ApplyToneIndex(void) {
  if (gCwToneIndex >= CW_TONE_PRESETS_COUNT)
    gCwToneIndex = CW_DEFAULT_TONE_INDEX;
  gCwToneFrequency = CW_TONE_PRESETS[gCwToneIndex];
}

void CW_LoadConfig(void) {
  uint8_t cfg[8];
  EEPROM_ReadBuffer(CW_EEPROM_CONFIG, cfg, sizeof(cfg));

  gCwWpm = (cfg[0] >= CW_WPM_MIN && cfg[0] <= CW_WPM_MAX) ? cfg[0]
                                                          : CW_DEFAULT_WPM;
  gCwToneIndex =
      (cfg[1] < CW_TONE_PRESETS_COUNT) ? cfg[1] : CW_DEFAULT_TONE_INDEX;
  CW_ApplyToneIndex();

  char msg[CW_MESSAGE_MAX];
  EEPROM_ReadBuffer(CW_EEPROM_MESSAGE, msg, sizeof(msg));

  // a blank (erased) cell -> fall back to the default message
  if ((uint8_t)msg[0] == 0xFF || msg[0] == 0x00) {
    strcpy(gCwMessage, CW_BEACON_MESSAGE);
    return;
  }

  for (uint8_t i = 0; i < CW_MESSAGE_MAX; i++) {
    const uint8_t c = (uint8_t)msg[i];
    if (c == 0xFF || c == 0x00) {
      gCwMessage[i] = 0;
      break;
    }
    gCwMessage[i] = msg[i];
    gCwMessage[i + 1] = 0;
  }
  gCwMessage[CW_MESSAGE_MAX] = 0;
}

void CW_SaveConfig(void) {
  uint8_t cfg[8];
  memset(cfg, 0xFF, sizeof(cfg));
  cfg[0] = gCwWpm;
  cfg[1] = gCwToneIndex;
  EEPROM_WriteBuffer(CW_EEPROM_CONFIG, cfg);

  char msg[CW_MESSAGE_MAX];
  memset(msg, 0xFF, sizeof(msg));
  for (uint8_t i = 0; i < CW_MESSAGE_MAX && gCwMessage[i] != 0; i++)
    msg[i] = gCwMessage[i];
  EEPROM_WriteBuffer(CW_EEPROM_MESSAGE, msg);
  EEPROM_WriteBuffer(CW_EEPROM_MESSAGE + 8, msg + 8);
}

void CW_SetWpm(uint8_t wpm) {
  if (wpm < CW_WPM_MIN) wpm = CW_WPM_MIN;
  if (wpm > CW_WPM_MAX) wpm = CW_WPM_MAX;
  gCwWpm = wpm;
  CW_SaveConfig();
}

void CW_SetToneIndex(uint8_t index) {
  gCwToneIndex = index;
  CW_ApplyToneIndex();
  CW_SaveConfig();
}

void CW_GetMessage(char *out) { strcpy(out, gCwMessage); }

void CW_SetMessage(const char *message) {
  strncpy(gCwMessage, message, CW_MESSAGE_MAX);
  gCwMessage[CW_MESSAGE_MAX] = 0;
  CW_SaveConfig();
}

// ---- Non-blocking keying state machine -----------------------------------
// The message is keyed as a strict alternation of phases driven every 10 ms:
//   ON phase  : the tone is keyed for 1 dot (dit) or 3 dots (dah)
//   OFF phase : a gap of 1 dot (between elements), 3 dots (between letters)
//               or 7 dots (between words)
static bool cw_active;
static char cw_buffer[CW_MESSAGE_MAX + 1];
static const char *cw_read;    // next character to fetch
static const char *cw_element; // position within the current character pattern
static uint16_t cw_dot_ticks;  // 10 ms units per dit
static uint16_t cw_countdown;  // ticks left in the current phase
static uint8_t cw_start_guard; // abort-suppression countdown
static bool cw_keyed;          // is the tone currently keyed on

static void CW_Stop(void) {
  BK4819_StopCW();
  RADIO_SendEndOfTransmission();
  FUNCTION_Select(FUNCTION_FOREGROUND);
  RADIO_SetupRegisters(true);

  cw_active = false;
  gUpdateStatus = true;
  gUpdateDisplay = true;
}

void CW_Abort(void) {
  if (cw_active) CW_Stop();
}

bool CW_IsActive(void) { return cw_active; }

void CW_Start(const char *message) {
  if (cw_active || gCurrentFunction == FUNCTION_TRANSMIT || message == NULL ||
      message[0] == 0)
    return;

  // make sure TX is allowed on the current VFO
  RADIO_SelectVfos();

  if (TX_freq_check(gCurrentVfo->pTX->Frequency) != 0 ||
      gBatteryDisplayLevel == 0 || gBatteryDisplayLevel > 6
#ifndef ENABLE_TX_WHEN_AM
      || gCurrentVfo->Modulation != MODULATION_FM
#endif
  ) {
    AUDIO_PlayBeep(BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL);
    return;
  }

  strncpy(cw_buffer, message, CW_MESSAGE_MAX);
  cw_buffer[CW_MESSAGE_MAX] = 0;

  cw_dot_ticks = (1200u / (gCwWpm > 0 ? gCwWpm : CW_DEFAULT_WPM) + 5) / 10;
  if (cw_dot_ticks == 0) cw_dot_ticks = 1;

  cw_read = cw_buffer;
  cw_element = NULL;
  cw_keyed = false;
  cw_countdown = 0;  // start the first element on the next tick
  cw_start_guard = CW_START_GUARD_TICKS;
  cw_active = true;

  // bring the transmitter up and arm the keying tone (left muted)
  FUNCTION_Select(FUNCTION_TRANSMIT);
  BK4819_StartCW(gCwToneFrequency, CW_TONE_LEVEL);
}

void CW_StartBeacon(void) { CW_Start(gCwMessage); }

// Key the next element of the current character, if any. Returns true if an
// element was started.
static bool CW_KeyNextElement(void) {
  if (cw_element == NULL || *cw_element == 0) return false;

  const char e = *cw_element++;
  BK4819_ExitTxMute();
  cw_keyed = true;
  cw_countdown = (e == '-') ? cw_dot_ticks * 3 : cw_dot_ticks;
  return true;
}

// Fetch and begin the next character. Returns false when the message is done.
static bool CW_BeginNextCharacter(void) {
  for (;;) {
    const char c = *cw_read;
    if (c == 0) return false;  // end of message
    cw_read++;

    if (c == ' ') {
      // word gap: 7 dots; 3 were already added after the previous letter
      cw_keyed = false;
      cw_element = NULL;
      cw_countdown = cw_dot_ticks * 4;
      return true;
    }

    const char *pattern = CW_Lookup(c);
    if (pattern == NULL) continue;  // skip unsupported characters

    cw_element = pattern;
    return CW_KeyNextElement();
  }
}

void CW_TimeSlice10ms(void) {
  if (!cw_active) return;

  // allow the operator to interrupt with PTT or any key
  if (cw_start_guard > 0) {
    cw_start_guard--;
  } else if (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT) ||
             KEYBOARD_Poll() != KEY_INVALID) {
    CW_Stop();
    return;
  }

  if (cw_countdown > 0) {
    cw_countdown--;
    return;
  }

  if (cw_keyed) {
    // an ON element just finished -> open an OFF gap
    BK4819_EnterTxMute();
    cw_keyed = false;

    if (cw_element != NULL && *cw_element != 0)
      cw_countdown = cw_dot_ticks;  // 1-dot gap before the next element
    else {
      cw_element = NULL;                // character finished
      cw_countdown = cw_dot_ticks * 3;  // 3-dot letter gap
    }
    return;
  }

  // an OFF gap just finished -> start the next element or character
  if (CW_KeyNextElement()) return;
  if (!CW_BeginNextCharacter()) CW_Stop();
}

#endif // ENABLE_CW
