/***************************************************************************//**
 * @file EFM32_CapSenseSlider.cpp
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

#include <mbed.h>
#include "platform.h"

#include "EFM32_CapSenseSlider.h"
#include "em_lesense.h"

namespace silabs {
    /*
     * Constructor.
     */
    EFM32_CapSenseSlider::EFM32_CapSenseSlider() :
        _channelCallback(this, &EFM32_CapSenseSlider::channelCallbackHandler),
        _scanCallback(this, &EFM32_CapSenseSlider::scanCallbackHandler)
    {
        _touchCb = NULL;
        _untouchCb = NULL;
        _slideCb = NULL;
        _trippingPoint = 0;
        _lastValue = -1;
        _position = -1;
        _touched = false;
        _running = false;
    }

    /*
     * Start measuring
     */
    void EFM32_CapSenseSlider::start() {
        if(_running == false) {
            CAPLESENSE_Init(true);
            CAPLESENSE_setupCallbacks((cbptr_t)_scanCallback.entry(), (cbptr_t)_channelCallback.entry());
            blockSleepMode(EM2);
            _running = true;
        }
    }

    /*
     * Stop measuring
     */
    void EFM32_CapSenseSlider::stop() {
        if(_running == true) {
            LESENSE_ScanStop();
            unblockSleepMode(EM2);
            _running = false;
        }
    }

    /*
     * Attach a callback handler, which gets called once on touch
     * callback: pointer to a void (void) function. If null, then the callback gets disabled.
     */
    void EFM32_CapSenseSlider::attach_touch(cbptr_t callback) {
        _touchCb = callback;
    }

    /*
     * Attach a callback handler, which gets called once on releasing touch
     * callback: pointer to a void (void) function. If null, then the callback gets disabled.
     */
    void EFM32_CapSenseSlider::attach_untouch(cbptr_t callback) {
        _untouchCb = callback;
    }

    /*
     * Attach a callback which will trigger once the slider value passes a certain point.
     *
     * trip: point accross which the callback gets called.
     * callback: pointer to a void (void) function. If null, then the callback gets disabled.
     */
    void EFM32_CapSenseSlider::attach_slide(int32_t trip, cbptr_t callback) {
        _slideCb = callback;
        _trippingPoint = trip;
    }

    /*
     * Check whether the slider is currently being touched.
     */
    bool EFM32_CapSenseSlider::isTouched() {
        return _touched;
    }

    /*
     * Get the current position
     */
    int32_t EFM32_CapSenseSlider::get_position() {
        return _position;
    }

    void EFM32_CapSenseSlider::channelCallbackHandler(void) {
        /* When a touch is detected, go to responsive scan mode */
        CAPLESENSE_setupLESENSE(false);
    }

    void EFM32_CapSenseSlider::scanCallbackHandler(void) {
        /* Calculate slider position */
        _position = CAPLESENSE_getSliderPosition();
        /* Check for touch */
        if(_position < 0) {
            /* Slider is no longer being touched */
            if(_touched == true) {
                _touched = false;
                if(_untouchCb != NULL) _untouchCb();
            }
            /* When no longer touched, go to sense mode */
            CAPLESENSE_setupLESENSE(true);
            return;
        }

        /* Touched, check if this is the first touch */
        if(_touched == false) {
            _touched = true;
            if(_touchCb != NULL) _touchCb();
        }

        /* Check if we tripped the threshold */
        if((_lastValue != _position) && (_slideCb != NULL)) {
            if((_trippingPoint == -1) || (_position >= _trippingPoint)) _slideCb();
        }

        _lastValue = _position;
    }
}
