/**
 * @file
 * @date 	28 April 2015
 * @author	Richard Osterloh <richard.osterloh@gmail.com>
 */

#include "mbed.h"

DigitalOut myled(LED1);
LowPowerTicker toggleTicker;

void ledToggler(void) {
    myled = !myled;
}

int main() {
    toggleTicker.attach(&ledToggler, 0.2f);
    while(1) {
        sleep();
    }
}
