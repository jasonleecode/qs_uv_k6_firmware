/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
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

#ifndef BATTERY_H
#define BATTERY_H

#include <stdbool.h>
#include <stdint.h>

extern uint16_t gBatteryCalibration[6];
extern uint16_t gBatteryCurrentVoltage;
extern uint16_t gBatteryCurrent;
extern uint16_t gBatteryVoltages[4];
extern uint16_t gBatteryVoltageAverage;
extern uint8_t gBatteryDisplayLevel;
extern bool gChargingWithTypeC;
extern bool gLowBatteryBlink;
extern bool gLowBattery;
extern bool gLowBatteryConfirmed;
extern uint16_t gBatteryCheckCounter;

extern volatile uint16_t gPowerSave_10ms;

typedef enum {
  BATTERY_TYPE_1600_MAH,
  BATTERY_TYPE_2200_MAH,
  BATTERY_TYPE_UNKNOWN
} BATTERY_Type_t;

unsigned int BATTERY_VoltsToPercent(unsigned int voltage_10mV);
void BATTERY_GetReadings(bool bDisplayBatteryLevel);
void BATTERY_TimeSlice500ms(void);

#endif
