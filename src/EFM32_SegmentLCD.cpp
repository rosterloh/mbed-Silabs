/***************************************************************************//**
 * @file EFM32_SegmentLCD.cpp
 * @brief Driver class for the segment LCD's on some of the EFM32 kits.
 *******************************************************************************
 * @section License
 * <b>(C) Copyright 2015 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#include <mbed.h>
#include "pinmap.h"
#include "EFM32_SegmentLCD.h"
#include "segmentlcd.h"
#include "sleep_api.h"

namespace silabs {
/*
 * Constructor.
 */
EFM32_SegmentLCD::EFM32_SegmentLCD() {
    /* Set all pins used for the LCD to disabled. */
    uint32_t num_pins = sizeof(outPins) / sizeof(outPins[0]);
    for(uint8_t i = 0; i < num_pins; i++) {
        pin_mode(outPins[i], Disabled);
    }

    /* Initialize the LCD without voltage booster */
    SegmentLCD_Init(false);

    /* Block sleep mode */
    blockSleepMode(EM2);
}

EFM32_SegmentLCD::~EFM32_SegmentLCD() {
    /* Shut off LCD peripheral */
    SegmentLCD_Disable();

    /* Unblock sleep mode */
    unblockSleepMode(EM2);
}

void EFM32_SegmentLCD::AllOff( void ) {
    SegmentLCD_AllOff();
}

void EFM32_SegmentLCD::AllOn( void ) {
    SegmentLCD_AllOn();
}

/*
 * Switch off (clear) the alphanumeric portion of the display
 */
void EFM32_SegmentLCD::AlphaNumberOff(void) {
    SegmentLCD_AlphaNumberOff();
}

/*
 * Switch specified segment on the ring on/off
 * anum: ring segment index
 * on: true to turn on, false to turn off
 */
void EFM32_SegmentLCD::ARing(int anum, bool on) {
    SegmentLCD_ARing(anum, (on ? 1 : 0));
}

/*
 * Display a battery level on the LCD.
 * 0 = off
 * 1 = lowest block
 * 2 = lowest + second-to-lowest
 * ...
 */
void EFM32_SegmentLCD::Battery(int batteryLevel) {
    SegmentLCD_Battery(batteryLevel);
}

/*
 * Display an energy mode ring on the LCD.
 * em = energy mode number to display
 * on = true to turn on, false to turn off.
 */
void EFM32_SegmentLCD::EnergyMode(int em, bool on) {
    SegmentLCD_EnergyMode(em, (on ? 1 : 0));
}

/*
 * Display an unsigned integer on the alphanumeric
 * portion of the display as a hex value.
 *
 * num = number to display
 */
void EFM32_SegmentLCD::LowerHex( uint32_t num ) {
    SegmentLCD_LowerHex(num);
}

/*
 * Display a signed integer as decimal number on
 * the alphanumeric part of the display.
 */
void EFM32_SegmentLCD::LowerNumber( int num ) {
    SegmentLCD_LowerNumber(num);
}

/*
 * Display a signed integer on the numeric part
 * of the display (clock area).
 * max = 9999, min = -9999
 */
void EFM32_SegmentLCD::Number(int value) {
    SegmentLCD_Number(value);
}

/*
 * Clear the numeric part of the display.
 */
void EFM32_SegmentLCD::NumberOff(void) {
    SegmentLCD_NumberOff();
}

/*
 * Turn a predefined symbol on or off.
 * lcdSymbol = predefined symbol in segmentlcdconfig_*.h
 * on = true to turn on, false to turn off.
 */
void EFM32_SegmentLCD::Symbol(lcdSymbol s, bool on) {
    SegmentLCD_Symbol(s, (on ? 1 : 0));
}

/*
 * Display an unsigned short integer as a hex value
 * on the numeric part of the display.
 * max = FFFF, min = 0
 */
void EFM32_SegmentLCD::UnsignedHex(uint16_t value) {
    SegmentLCD_UnsignedHex(value);
}

/*
 * Display a 7-character string on the alphanumeric
 * portion of the display.
 */
void EFM32_SegmentLCD::Write(char *string) {
    SegmentLCD_Write(string);
}


}
