/**
 * @file
 * @date 	28 April 2015
 * @author	Richard Osterloh <richard.osterloh@gmail.com>
 */

#include "mbed.h"
#include "EFM32_SegmentLCD.h"
#include "EFM32_CapSenseSlider.h"

/******************** Define I/O *****************************/
InterruptIn in(PB9);

silabs::EFM32_SegmentLCD segmentDisplay;
silabs::EFM32_CapSenseSlider capSlider;

/******************** Define Timers *****************************/
LowPowerTicker refreshTicker;

/***************** Define global variables **********************/
#define INIT_SECONDS        17600
#define TEST_DURATION       10

volatile uint32_t count = 0, seconds = INIT_SECONDS;


/***************** Define callback handlers *********************/
void tickerCallback(void);
void in_handler();
void touchCallback(void);
void slideCallback(void);

/**
 * Callback for pushbutton interrupt
 */
void in_handler() {
    count++;
    segmentDisplay.ARing(count & 0x7, (count & 0x8) == 0);
}

/**
 * Callback for 1 second timebase
 */
void tickerCallback(void) {
    seconds++;
    uint32_t clockValue = ((seconds / 60) % 60) * 100 + (seconds % 60);
    segmentDisplay.Number(clockValue);
    segmentDisplay.Symbol(LCD_SYMBOL_COL10, seconds & 0x1);
}

/**
 * Callback for touching/untouching the cap slider
 */
void touchCallback(void) {
    segmentDisplay.Symbol(LCD_SYMBOL_EFM32, capSlider.isTouched());

    if(!capSlider.isTouched()) {
        segmentDisplay.Write("Hello");
    }
}

/**
 * Callback for sliding the cap slider
 */
void slideCallback(void) {
    segmentDisplay.LowerNumber(capSlider.get_position());
}

/*************************** MAIN *******************************/
int main() {
    // Initialize pushbutton handler
    in.rise(NULL);
    in.fall(in_handler);

    // Enable the capacitive slider
    capSlider.start();
    capSlider.attach_touch(touchCallback);
    capSlider.attach_untouch(touchCallback);
    capSlider.attach_slide(-1, slideCallback);

    // Start generating the 1Hz timebase
    refreshTicker.attach(&tickerCallback, 1.0f);

    printf("Initialization done! \n");
    wait(0.01f); //Need to delay slightly to give the serial transmission a chance to flush out its buffer
    segmentDisplay.Write("Hello");

    // Go into sleeping mode
    while(1)
    {
        sleep();
        if(count >= 8) {
            // Go to 'completely dead' mode to show power consumption
            refreshTicker.detach();
            capSlider.stop();
            in.disable_irq();
            delete &segmentDisplay;
        }
    }
}
