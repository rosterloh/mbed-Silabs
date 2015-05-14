/***************************************************************************//**
 * @file EFM32_CapSenseSlider.h
 * @brief Driver class for the capacitive touch slider on some EFM32 STK's.
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

#ifndef SILABS_EFM32_CAPSENSESLIDER_H
#define SILABS_EFM32_CAPSENSESLIDER_H

#ifndef TARGET_EFM32
#error "The Silicon Labs EFM32 CapSenseSlider library is specifically designed for EFM32 targets."
#else
#include "platform.h"
#include <mbed.h>

#include "caplesense.h"
#include "CThunk.h"
#include "sleepmodes.h"

typedef void (*cbptr_t)(void);

namespace silabs {

/**  A driver for the capacitive touch slider on some EFM32 STKs
 *
 * Currently supports EFM32 Wonder, Giant and Leopard Gecko kits.
 *
 * @code
 * #include "mbed.h"
 * #include "EFM32_CapSenseSlider.h"
 *
 * silabs::EFM32_CapSenseSlider capSlider;
 *
 * void touchCallback(void) {
 *   if(!capSlider.isTouched()) {
 *       printf("Lost touch");
 *   } else {
 *       printf("Finger detected! Position %d", capSlider.getPosition());
 *   }
 * }
 *
 * int main() {
 *     capSlider.start();
 *     capSlider.attach_touch(touchCallback);
 *
 *     while(1) sleep();
 * }
 * @endcode
 */
class EFM32_CapSenseSlider {
public:
    /**
     * Constructor.
     */
    EFM32_CapSenseSlider();

    /**
     * Start measuring
     */
    void start();

    /**
     * Stop measuring
     */
    void stop();

    /**
     * Attach a callback handler, which gets called once on touch
     *
     * @param callback   pointer to a void (void) function. If null, then the callback gets disabled.
     */
    void attach_touch(cbptr_t callback = NULL);

    /**
     * Attach a callback handler, which gets called once on releasing touch
     *
     * @param callback   pointer to a void (void) function. If null, then the callback gets disabled.
     */
    void attach_untouch(cbptr_t callback = NULL);

    /**
     * Attach a callback which will trigger once the slider value passes a certain point.
     *
     * @param trip       point after which the callback gets called. If -1, the callback gets called on any change in position.
     * @param callback   pointer to a void (void) function. If null, then the callback gets disabled.
     */
    void attach_slide(int32_t trip = -1, cbptr_t callback = NULL);

    /**
     * Check whether the slider is currently being touched.
     *
     * @return           True if a finger is currently detected.
     */
    bool isTouched();

    /**
     * Get the current position
     *
     * @return           The finger position on the slider (0-48). -1 if not touched.
     */
    int32_t get_position();

protected:
    CThunk<EFM32_CapSenseSlider> _channelCallback;
    CThunk<EFM32_CapSenseSlider> _scanCallback;

    cbptr_t _slideCb;
    cbptr_t _touchCb;
    cbptr_t _untouchCb;
    int32_t _trippingPoint;
    bool _running;
    bool _touched;
    volatile int32_t _lastValue, _position;

    void channelCallbackHandler(void);
    void scanCallbackHandler(void);

};
}
#endif //TARGET Check

#endif //SILABS_EFM32_CAPSENSESLIDER_H
