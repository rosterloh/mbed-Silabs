/**
 * @file
 * @date 	28 April 2015
 * @author	Richard Osterloh <richard.osterloh@gmail.com>
 */

#include "mbed.h"
//#include "EFM32_SegmentLCD.h"
//#include "EFM32_CapSenseSlider.h"

/******************** Define I/O *****************************/
InterruptIn in(PB9);
DigitalOut LED(LED0);
Serial pc(USBTX, USBRX);

//silabs::EFM32_SegmentLCD segmentDisplay;
//silabs::EFM32_CapSenseSlider capSlider;

/******************** Define Timers *****************************/
LowPowerTicker refreshTicker;

/***************** Define global variables **********************/
#define INIT_SECONDS        17600
//#define TEST_DURATION       10
#define BUFF_LENGTH         5

volatile uint32_t count = 0, seconds = INIT_SECONDS;
event_callback_t    serialEventCb;
uint8_t             rx_buf[BUFF_LENGTH + 1];

/***************** Define callback handlers *********************/
void in_handler();
void tickerCallback(void);
//void touchCallback(void);
//void slideCallback(void);
void serialCb(int events);

/**
 * Callback for pushbutton interrupt
 */
void in_handler() {
    count++;
//    segmentDisplay.ARing(count & 0x7, (count & 0x8) == 0);
}

/**
 * Callback for 1 second timebase
 */
void tickerCallback(void) {
    seconds++;
//    uint32_t clockValue = ((seconds / 60) % 60) * 100 + (seconds % 60);
//    segmentDisplay.Number(clockValue);
//    segmentDisplay.Symbol(LCD_SYMBOL_COL10, seconds & 0x1);
}

/**
 * Callback for touching/untouching the cap slider
 */
//void touchCallback(void) {
//    segmentDisplay.Symbol(LCD_SYMBOL_EFM32, capSlider.isTouched());

//    if(!capSlider.isTouched()) {
//        segmentDisplay.Write("Hello");
//    }
//}

/**
 * Callback for sliding the cap slider
 */
//void slideCallback(void) {
//    segmentDisplay.LowerNumber(capSlider.get_position());
//}

/**
 * Callback for serial port
 */
void serialCb(int events) {
    /* Something triggered the callback, either buffer is full or 'S' is received */
    unsigned char i = 0;
    if(events & SERIAL_EVENT_RX_CHARACTER_MATCH) {
        //Received 'S', check for buffer length
        for(i = 0; i < BUFF_LENGTH; i++) {
            //Found the length!
            if(rx_buf[i] == 'S') break;
        }
    } else if (events & SERIAL_EVENT_RX_COMPLETE) {
        i = BUFF_LENGTH - 1;
    } else {
        rx_buf[0] = 'E';
        rx_buf[1] = 'R';
        rx_buf[2] = 'R';
        rx_buf[3] = '!';
        rx_buf[4] = 0;
        i = 3;
    }

    // Echo string, no callback
    pc.write(rx_buf, i+1, 0, 0);

    // Reset serial reception
    pc.read(rx_buf, BUFF_LENGTH, serialEventCb, SERIAL_EVENT_RX_ALL, 'S');
}

/*************************** MAIN *******************************/
int main() {
    // Initialize pushbutton handler
    in.rise(NULL);
    in.fall(in_handler);

    // Enable the capacitive slider
    //capSlider.start();
    //capSlider.attach_touch(touchCallback);
    //capSlider.attach_untouch(touchCallback);
    //capSlider.attach_slide(-1, slideCallback);

    // Start generating the 1Hz timebase
    refreshTicker.attach(&tickerCallback, 1.0f);

    //printf("Initialization done! \n");
    //wait(0.01f); //Need to delay slightly to give the serial transmission a chance to flush out its buffer
    //segmentDisplay.Write("Hello");

    serialEventCb.attach(serialCb);

    /* Setup serial connection */
    pc.baud(115200);
    pc.printf("Low Power API test\r\n\nSend 'S' to toggle blinking\r\n");
    pc.read(rx_buf, BUFF_LENGTH, serialEventCb, SERIAL_EVENT_RX_ALL, 'S');

    // Go into sleeping mode
    while(1)
    {
        sleep();
        //if(count >= 8) {
            // Go to 'completely dead' mode to show power consumption
        //    refreshTicker.detach();
        //    capSlider.stop();
        //    in.disable_irq();
        //    delete &segmentDisplay;
        //}
    }
}
