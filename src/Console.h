/**
 * @file
 * The console class accepts and executes commands as well
 * as provides debug output for the rest of the system.
 *
 * @author  Richard Osterloh <richard.osterloh@gmail.com>
 * @date    20/05/2015
 */

#pragma once

#include "mbed.h"
#include "rtos.h"

/**
 * @class   Console
 * @brief   a console and logging object.
 * @details This class receives commands from the serial
 *          port, processes and executes them while also
 *          providing logging facilities to the rest of the
 *          system
 */
class Console {
public:
  enum Constants {
      MessageLen  = 80,
      NumMessages = 16,
      InBufferLen = 32
  };

  /**
   * Console object constructor
   * @param tx transmit pin
   * @param rx receive pin
   */
  Console(PinName tx, PinName rx);
  //~Console();

  //int printf(const char* format, ...);
  //int isr_printf(const char* format, ...);

private:
  //typedef struct {
  //    char*   ptr;                 //!< Non-NULL if memory is allocated
  //    char    msg[MessageLen+1];   //!< A counter value
  //} message_t;

  //MemoryPool<message_t, NumMessages> _mpool;
  //Queue<message_t, NumMessages> _queue;
  event_callback_t _serialEventCb;
  //Semaphore _sem;
  Serial _serial;
  Thread _thr;
  unsigned char _rxBuffer[InBufferLen];	//!< RX ring buffer
  bool _messageReceived;		//!< a complete message is waiting in the buffer

  static void logTask(void const* args);
  void printHelp(void);
  void messageProcess(void);
  void serialCb(int events);
};
