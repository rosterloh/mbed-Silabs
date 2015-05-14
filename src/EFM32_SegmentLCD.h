/***************************************************************************//**
 * @file EFM32_SegmentLCD.h
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

#ifndef SILABS_SEGMENTLCD_H
#define SILABS_SEGMENTLCD_H

#ifndef TARGET_EFM32
#error "The Silicon Labs SegmentLCD library is specifically designed for EFM32 targets."
#else
#include "platform.h"
#include <mbed.h>

#include "segmentlcd.h"
#include "sleepmodes.h"

namespace silabs {
/** A driver for the Segment LCD present on some EFM32 starter kits
 *
 * Supports Giant, Leopard and Wonder Gecko STKs
 *
 * @code
 * #include "mbed.h"
 * #include "EFM32_SegmentLCD.h"
 *
 * EFM32_SegmentLCD segmentDisplay;
 *
 * int main() {
 *     segmentDisplay.Write("Hello");
 *     while(1);
 * }
 * @endcode
 */

class EFM32_SegmentLCD {
public:
    /**
     * Constructor.
     */
    EFM32_SegmentLCD();

    /**
     * Destructor.
     */
    ~EFM32_SegmentLCD();

    /**
     * Switch off all elements
     */
    void AllOff( void );

    /**
     * Switch on all elements
     */
    void AllOn( void );

    /**
     * Switch off (clear) the alphanumeric portion of the display
     */
    void AlphaNumberOff(void);

    /**
     * Switch specified segment on the ring on/off
     *
     * @param anum  ring segment index
     * @param on    true to turn on, false to turn off
     */
    void ARing(int anum, bool on);

    /**
     * Display a battery level on the LCD.
     * 0 = off
     * 1 = lowest block
     * 2 = lowest + second-to-lowest
     * ...
     *
     * @param batteryLevel  Level to show
     */
    void Battery(int batteryLevel);

    /**
     * Display an energy mode ring on the LCD.
     *
     * @param em  energy mode number to display
     * @param on  true to turn on, false to turn off.
     */
    void EnergyMode(int em, bool on);

    /**
     * Display an unsigned integer on the alphanumeric
     * portion of the display as a hex value.
     *
     * @param num number to display
     */
    void LowerHex( uint32_t num );

    /**
     * Display a signed integer as decimal number on
     * the alphanumeric part of the display.
     *
     * @param num  number to display
     */
    void LowerNumber( int num );

    /**
     * Display a signed integer on the numeric part
     * of the display (clock area).
     * max = 9999, min = -9999
     */
    void Number(int value);

    /**
     * Clear the numeric part of the display.
     */
    void NumberOff(void);

    /**
     * Turn a predefined symbol on or off.
     *
     * @param lcdSymbol  predefined symbol in segmentlcdconfig_*.h
     * @param on         true to turn on, false to turn off.
     */
    void Symbol(lcdSymbol s, bool on);

    /**
     * Display an unsigned short integer as a hex value
     * on the numeric part of the display.
     * max = FFFF, min = 0
     *
     * @param value      Value to show
     */
    void UnsignedHex(uint16_t value);

    /**
     * Display a 7-character string on the alphanumeric
     * portion of the display.
     *
     * @param string     String to show (only the first 7 characters will be shown)
     */
    void Write(char *string);
protected:

};
}

#endif //TARGET_EFM32
#endif //SILABS_SEGMENTLCD_H
