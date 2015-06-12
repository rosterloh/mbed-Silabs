/**
 * @file
 * @date 	14 May 2015
 * @author	Richard Osterloh <richard.osterloh@gmail.com>
 */

#include "mbed.h"
#include "rtos.h"
//#include "Console.h"
//#include "Modem.h"
#include "SerialRPCInterface.h"

/*------------ Constant definitions --------------*/

/*------------------ Variables -------------------*/
DigitalOut          LED_G(PE2/*PB0*/);
DigitalOut          LED_B(PE3/*PB1*/);
DigitalOut          LED_R(PB2);

/*------------------ Callbacks -------------------*/
//void led2_thread(void const *args) {
//    while (true) {
//	LED_B = !LED_B;
//        Thread::wait(1000);
//    }
//}

/*-------------------- Main ----------------------*/
int main() {
    //Console log(PD0, PD1);

    SerialRPCInterface rpc(USBTX, USBRX, 115200);
    //Thread thread(led2_thread);

    while (true) {
	LED_G = !LED_G;
        Thread::wait(500);
    }

}
