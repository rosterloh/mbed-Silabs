/**
 * @file
 * The console class accepts and executes commands as well
 * as provides debug output for the rest of the system.
 *
 * @author  Richard Osterloh <richard.osterloh@gmail.com>
 * @date    20/05/2015
 */

#include "mbed.h"
#include "Console.h"
//#include <cstdarg>

void Console::logTask(void const* args)
{
    Console* instance = (Console*)args;

    instance->printHelp();

    while (true) {
/*
	osEvent evt = instance->_queue.get();
        if (evt.status == osEventMessage) {
            message_t *message = (message_t*)evt.value.p;
            if (message->ptr != NULL) {
                instance->_serial.printf(message->ptr);
                free(message->ptr);
            } else {
                instance->_serial.printf(message->msg);
            }

            instance->_mpool.free(message);

            //!< Increase the number of available messages in the pool
            instance->_sem.release();
        }
*/
        //!< process console input commands
        if(instance->_messageReceived)  instance->messageProcess();
    }
}

void Console::printHelp(void) {
    //Console::printf("\r\nEFM32GG build " __DATE__ " " __TIME__ "\r\n");
  _serial.printf("\r\nEFM32GG build " __DATE__ " " __TIME__ "\r\n");
}

void Console::messageProcess(void) {
    /*if (!strncmp(_rxBuffer, "help", sizeof("help")-1)) {
        printHelp();
    } else if (!strncmp(_rxBuffer, "get ", sizeof("get ")-1)) {
        printf("\r\nGET recognised");
    } else if (!strncmp(_rxBuffer, "set ", sizeof("set ")-1)) {
        printf("\r\nSET recognised");
    } else printf("\r\nUnkown command");
    */
    _serial.printf("Message received!");
    _messageReceived = false;
}

void Console::serialCb(int events) {
    /* Something triggered the callback, either buffer is full or '\n' is received */
    //unsigned char i = 0;
    if(events & SERIAL_EVENT_RX_CHARACTER_MATCH) {
        //Received '\n', check for buffer length
        //for(unsigned char i; i < InBufferLen; i++) {
            //Found the length!
        //    if(_rxBuffer[i] == '\n') break;
        //}
        _messageReceived = true;
    } else if (events & SERIAL_EVENT_RX_COMPLETE) {
        //i = InBufferLen - 1;
    } else {
	// TODO: Handle more errors
    }

    // Reset serial reception
    _serial.read(_rxBuffer, InBufferLen, _serialEventCb, SERIAL_EVENT_RX_ALL, '\n');
}

Console::Console(PinName tx, PinName rx) :
    //_sem(NumMessages),
    _serial(tx, rx),
    _thr(Console::logTask, this)
{
    _messageReceived = false;

    _serialEventCb.attach(this, &Console::serialCb);

    _serial.baud(115200);
    _serial.read(_rxBuffer, InBufferLen, _serialEventCb, SERIAL_EVENT_RX_ALL, '\n');
}
/*
Console::~Console()
{
    if (_thr != NULL) {
        _thr->terminate();
        delete _thr;
        _thr = NULL;
    }
}
*/
/*
int Console::printf(const char* format, ...)
{
    // The pool has no "wait for free message" so we use a Sempahore
    //  to keep track of the number of free messages and, if needed,
    //  block the caller until a message is free
    _sem.wait();

    // Allocate a null terminated message. Will always succeed due to
    // the semaphore above
    message_t *message = _mpool.calloc();

    // Write the callers formatted message
    std::va_list args;
    va_start(args, format);
    int ret = vsnprintf(message->msg, MessageLen, format, args);
    va_end(args);

    // If the entire message could not fit in the preallocated buffer
    //  then allocate a new one and try again.
    if (ret > MessageLen) {
        message->ptr = (char*)malloc(ret + 1);
        if (message->ptr != NULL) {
            va_start(args, format);
            ret = vsnprintf(message->ptr, ret + 1, format, args);
            va_end(args);
        }
    }

    // Send message
    _queue.put(message);

    // Note that the Semaphore is not released here, that is done after
    // the message has been processed and released into the pool by logTask()
    //_sem.release();

    return ret;
}

int Console::isr_printf(const char* format, ...)
{
    // The pool has no "wait for free message" so we use a Sempahore
    // to keep track of the number of free messages and, if needed,
    // block the caller until a message is free
    int available = _sem.wait(0);
    if (available <= 0) {
      // no free messages and it is not good to wait in an ISR so
      // we discard the message
      return 0;
    }

    // Allocate a null terminated message. Will always succeed due to
    // the semaphore above
    message_t *message = _mpool.calloc();

    // Write the callers formatted message
    std::va_list args;
    va_start(args, format);
    int ret = vsnprintf(message->msg, MessageLen, format, args);
    va_end(args);

    // If the entire message could not fit in the preallocated buffer
    // then allocate a new one and try again.
    if (ret > MessageLen) {
        message->ptr = (char*)malloc(ret + 1);
        if (message->ptr != NULL) {
            va_start(args, format);
            ret = vsnprintf(message->ptr, ret + 1, format, args);
            va_end(args);
        }
    }

    // Send message
    _queue.put(message);

    // Note that the Semaphore is not released here, that is done after
    // the message has been processed and released into the pool by
    // logTask()
    //_sem.release();

    return ret;
}
*/
