/*
// set the channel in the range 0-40// example: 0xAA 0x55 0x07(Command 3) 0x01(Length) 0x00(All 40 Channels 0-40) 0xB8(CRC8
 * SPI driver based on fs_skyrf_58g-main.c Written by Simon Chambers
 * TVOUT by Myles Metzel
 * Scanner by Johan Hermen
 * Inital 2 Button version by Peter (pete1990)
 * Refactored and GUI reworked by Marko Hoepken
 * Universal version my Marko Hoepken
 * Diversity Receiver Mode and GUI improvements by Shea Ivey
 * OLED Version by Shea Ivey
 * Seperating display concerns by Shea Ivey
The MIT License (MIT)
Copyright (c) 2015 Marko Hoepken
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "settings.h"
#include "settings_internal.h"
#include "settings_eeprom.h"

#include "channels.h"
#include "receiver.h"
#include "receiver_spi.h"
#include "buttons.h"
#include "state.h"

#include "ui.h"

#include <SoftwareSerial.h>

const byte rxPin = 2;
const byte txPin = 3;


// Set up a new SoftwareSerial object
SoftwareSerial mySerial1 (sxPin, sxPin, 4800);
SoftwareSerial mySerial2 (rxPin, txPin, 4800);


static void globalMenuButtonHandler(
    Button button,
    Buttons::PressType pressType
);


Stream* vtx;
Stream* fc;

/*
// set the frequency in the range 5000-6000 MHz
// Example: 0xAA 0x55 0x09(Command 4) 0x02(Length) 0x16 0xE9(Frequency 5865) 0xDC(CRC8
void setOutFrequency(int frequency) {
  byte a = frequency / 0x100;
  byte b = frequency % 0x100;
  byte * c = {0x00,0x00,0xAA,0x55,0x09,0x02,a,b,0x00};
  String m = "VTX Frequency: ${frequency}" ;
  sa->write(c, 9);
  log->print(m);
  int x = getOutSetting(1) ;
  String y = "VTX Actual Frequency: ${x}";
  log->print(y);
}
*/

// set the channel in the range 0-40// example: 0xAA 0x55 0x07(Command 3) 0x01(Length) 0x00(All 40 Channels 0-40) 0xB8(CRC8
void setOutChannel(byte channel) {
  byte* b =  {0x00,0x00,0xAA,0x55,0x07,0x01,channel,0x00,0x00};
  //String m = "VTX Channel: ${channel}"; 
  vtx->write(b, 9);
  //log->print(m);
    
  //int x = getOutSetting(0) ;
  //String y = "VTX Actual Channel: ${x}";
  //log->print(y);
}


void setup()
{
    Serial.begin(115200);
    vtx = &(mySerial1);
    fc  = &(mySerial2);
	
    setupPins();
    setupSettings();

    StateMachine::setup();
    Receiver::setup();
    Ui::setup();

    Receiver::setActiveReceiver(Receiver::ReceiverId::A);
}

void setupPins() {
    //pinMode(PIN_LED_A,OUTPUT);
    #ifdef USE_DIVERSITY
        //pinMode(PIN_LED_B,OUTPUT);
    #endif

    pinMode(PIN_RSSI_A, INPUT_PULLUP);
    #ifdef USE_DIVERSITY
        pinMode(PIN_RSSI_B, INPUT_PULLUP);
    #endif

    pinMode(PIN_SPI_SLAVE_SELECT, OUTPUT);
    pinMode(PIN_SPI_DATA, OUTPUT);
    pinMode(PIN_SPI_CLOCK, OUTPUT);

    digitalWrite(PIN_SPI_SLAVE_SELECT, HIGH);
    digitalWrite(PIN_SPI_CLOCK, LOW);
    digitalWrite(PIN_SPI_DATA, LOW);
}

void setupSettings() {
    EepromSettings.load();
    Receiver::setChannel(EepromSettings.startChannel);
}

void loop() {
    Receiver::update();
    StateMachine::update();
    Ui::update();
    EepromSettings.update();

    // read command and if that is SET_CHANNEL set value
    // it as vrx channel and set "some different" to vtx  
    if(sa->available()) {
      byte a = fc->readByte();
	    
      if(a == 0xAA) {
        byte b = fc->readByte();
        
	if(b == 0x55) {
	  byte c = fc->readByte();

	  if(c == 0x07) {
            fc->readByte();

	    // channel set by fc, e.g. channel of source vehicle vtx
	    byte d = fc->readByte();
            Receiver::setChannel(d);

	    // simply set out vtx with different channel from what fc set to vrx
	    byte e = random(0,40);
            while(e == d) {
	      e = random(0,40);
	    }

	    setOutChannel(e);
	  }
	}
      }
    }
}


static void globalMenuButtonHandler(
    Button button,
    Buttons::PressType pressType
) {
    /*
    if (
        StateMachine::currentState != StateMachine::State::MENU &&
        button == Button::MODE &&
        pressType == Buttons::PressType::HOLDING
    ) {
        StateMachine::switchState(StateMachine::State::MENU);
    }
    */
    // TODO talk to fc
    
}


