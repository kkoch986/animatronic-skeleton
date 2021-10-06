#include "Arduino-Renard.h"

//Captures address and size of buffer
void Renard::begin(uint8_t * ptr, uint8_t length, HardwareSerial *theSerial, uint32_t baud){
	address = ptr;
	size = length;
	_serial = theSerial;
	rx_state = 0;
	rx_channel =0;
	
	_serial->begin(baud);
}

boolean Renard::receive(){

	//create a return flag
	uint8_t rtn = false;
	uint8_t ren_temp = 0;
	uint8_t raw = 0;

	//for now let's block till we empty the serial buffer
	while(_serial->available() > 0){
		switch(rx_state){
			case WAITING: //look for sync
				raw = _serial->read();
				_serial->print("WAITING read: ");
				_serial->println(raw, HEX);
				if (raw == 0x7E){
					_serial->println("WAITING -> ADDRESS");
					rx_state = ADDRESS;
				}			
			break;
			case ADDRESS: //look for my address
				ren_temp = _serial->read();
				_serial->print("ADRESS read: ");
				_serial->println(ren_temp, HEX);
				if (ren_temp >= 0x80){
					rx_state = STRIPPING;
					rx_channel = 8 * (ren_temp - 0x80);
					_serial->print("ADDRESS -> STRIPPING @ ");
					_serial->println(rx_channel, HEX);

					if(rx_channel > size) {
						_serial->println("RX_CHAN exceeds size, moving to FORWARDING");
						rx_state = FORWARDING;
					}
				}
			break;
			case STRIPPING: //peel off my data
				ren_temp = _serial->read();
				_serial->print("STRIP: ");
				_serial->println(ren_temp, HEX);

				if (ren_temp == 0x7D) { //pad byte, drop
					_serial->println("DROP PAD BYTE");
					break;
				} else if (ren_temp == 0x7F) { //escape byte
					_serial->println("STRIPPING -> ESCAPE");
					rx_state = ESCAPE;
				} else if (ren_temp == 0x7E) { // starting a new frame
					_serial->println("STRIPPING -> ADDRESS");
					rx_state = ADDRESS;
					rtn = true;
				} else {
					//write brightness value into memory
					_serial->print("STORE ");
					_serial->print(ren_temp, HEX);
					_serial->print(" @ ");
					_serial->println(rx_channel);
					*(address+rx_channel++) = ren_temp;
					
					//check if last value	
					if(rx_channel == size){
						_serial->println("STRIPPING -> FORWARDING");
						rx_state = FORWARDING;
						rtn = true;
					}			
				}							
			break;
			case ESCAPE: //peel off escaped byte			
				raw = _serial->read();
				_serial->print("ESCAPE read: ");
				_serial->println(raw, HEX);
				*(address+rx_channel++) = raw + 0x4E;
				if(rx_channel >= size){
					rx_state = FORWARDING;						
					rtn = true;
				} else {
					rx_state = STRIPPING;
				}
			break;
			case FORWARDING: //forward rest
				ren_temp = _serial->read();
				_serial->print("FORWARDING read: ");
				_serial->println(ren_temp, HEX);
				if(ren_temp == 0x7E) {
					rx_state = ADDRESS;
				}
			break;
		}    
		_serial->println("Exiting loop");
	}
	return rtn;
}

void RenardTX::begin(uint16_t length, HardwareSerial *theSerial, uint32_t baud){
	
	_size = length;
	_serial = theSerial;
	_serial->begin(baud);
}

void RenardTX::startPacket(){
	
	_ptr = (uint8_t*) malloc(_size+2);
	_ptr[0] = 0x7E;
	_ptr[1] = 0x80;
	
	
}

void RenardTX::setValue(uint16_t address, uint8_t value){
	
	switch (value){
		case 0x7d:
		_ptr[address+2] = 0x7c;
		break;
		case 0x7e:
		case 0x7f:
		_ptr[address+2] = 0x80;
		break;
		default:
		_ptr[address+2] = value;
		break;
	}
}

void RenardTX::sendPacket(){
	
	_serial->write(_ptr, _size+2);
	_serial->flush();
	
	free(_ptr);
}
