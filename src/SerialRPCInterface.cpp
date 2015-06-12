 /**
* @section LICENSE
*Copyright (c) 2010 ARM Ltd.
*
*Permission is hereby granted, free of charge, to any person obtaining a copy
*of this software and associated documentation files (the "Software"), to deal
*in the Software without restriction, including without limitation the rights
*to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*copies of the Software, and to permit persons to whom the Software is
*furnished to do so, subject to the following conditions:
*
*The above copyright notice and this permission notice shall be included in
*all copies or substantial portions of the Software.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*THE SOFTWARE.
*
*
* @section DESCRIPTION
*
*This class sets up RPC communication. This allows objects on mbed to be controlled. Objects can be created or existing objects can be used
*/
#include "SerialRPCInterface.h"

using namespace mbed;

//Requires multiple constructors for each type, serial to set different pin numbers, TCP for port.
SerialRPCInterface::SerialRPCInterface(PinName tx, PinName rx, int baud) :
	_serial(tx, rx),
	_thread(SerialRPCInterface::_MsgProcessStarter, this, osPriorityNormal, 0/*1024*/) {
    //_RegClasses();
    _enabled  = true;
    _command_ptr = _command;
    _RPCflag = false;
    //pc.attach(this, &SerialRPCInterface::_RPCSerial, Serial::RxIrq);
    _serialEventCb.attach(this, &SerialRPCInterface::serialCb);
    //_serial.baud(baud);
    _serial.printf("---- RPC interface build " __DATE__ " " __TIME__ "----");
    _serial.read(rx_buf, RPC_MAX_STRING, _serialEventCb, SERIAL_EVENT_RX_ALL, '\n');
}

void SerialRPCInterface::_RegClasses(void){
    //Register classes with base
    //RPC::add_rpc_class<RpcAnalogIn>();
    //RPC::add_rpc_class<RpcAnalogOut>();
    RPC::add_rpc_class<RpcDigitalIn>();
    RPC::add_rpc_class<RpcDigitalOut>();
    RPC::add_rpc_class<RpcDigitalInOut>();
    //RPC::add_rpc_class<RpcPwmOut>();
    //RPC::add_rpc_class<RpcTimer>();
    //RPC::add_rpc_class<RpcBusOut>();
    //RPC::add_rpc_class<RpcBusIn>();
    //RPC::add_rpc_class<RpcBusInOut>();
    //RPC::add_rpc_class<RpcSerial>();
    //RPC::add_rpc_class<RpcSPI>();
    //RPC::add_rpc_class<RpcI2C>();
}

void SerialRPCInterface::Disable(void){
    _enabled = false;
}
void SerialRPCInterface::Enable(void){
    _enabled = true;
}
void SerialRPCInterface::_MsgProcessStarter(const void *args){
    SerialRPCInterface *instance = (SerialRPCInterface*)args;
    while(1){
        instance->_MsgProcess();
    }
}
void SerialRPCInterface::_MsgProcess(void) {
  /*
    osEvent evt = _commandMail.get();
    if(evt.status == osEventMail){
        mail_t *mail = (mail_t*)evt.value.p;
        if(_enabled == true){
            RPC::call(_command, _response);
            _serial.printf("%s\n", _response);
        }
        _commandMail.free(mail);
    }
    */
}

void SerialRPCInterface::_RPCSerial() {
//    _RPCflag = true;
//    if(_enabled == true){
//        char c = 0/*pc.getc()*/;
//        if(c == 13){
//            *_command_ptr = '\0';
//            mail_t *mail = _commandMail.alloc();
//            strcpy(mail->command, _command);
//            _commandMail.put(mail);
//            _command_ptr = _command;
//        }else if(_command_ptr - _command < 256){
//            *_command_ptr = c;
//            _command_ptr++;
//        }
//    }
//    _RPCflag = false;
}

void SerialRPCInterface::serialCb(int events) {
    /* Something triggered the callback, either buffer is full or 'S' is received */
    unsigned char i = 0;
    if(events & SERIAL_EVENT_RX_CHARACTER_MATCH) {
        //Received '\n', check for buffer length
        _serial.printf("SERIAL_EVENT_RX_CHARACTER_MATCH");
    } else if (events & SERIAL_EVENT_RX_COMPLETE) {
        _serial.printf("SERIAL_EVENT_RX_COMPLETE");
    } else {
	rx_buf[0] = 'E';
	rx_buf[1] = 'R';
	rx_buf[2] = 'R';
	rx_buf[3] = '!';
	rx_buf[4] = 0;
	i = 3;
    }

    // Echo string, no callback
    _serial.write(rx_buf, i+1, 0, 0);

    // Reset serial reception
    _serial.read(rx_buf, RPC_MAX_STRING, _serialEventCb, SERIAL_EVENT_RX_ALL, '\n');
}
