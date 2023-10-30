/*
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
const byte saPin = 3;


// Set up a new SoftwareSerial object

SoftwareSerial saSerial (rxPin, saPin, 4800);


static void globalMenuButtonHandler(
    Button button,
    Buttons::PressType pressType
);


Stream* sa;
Stream* log;
Stream* pad;

// returns setting value ; 0 - channel, 1 - frequency, 2 - version. returns -1 in case of read failure 
// smartAudio V1 response: VTX: 0xAA 0x55 0x01 (Version/Command) 0x06 (Length) 0x00 (Channel) 0x00 (Power Level) 0x01(OperationMode) 0x16 0xE9(Current Frequency 5865) 0x4D(CRC8)
// smartAudio V2 response: VTX: 0xAA 0x55 0x09 (Version/Command) 0x06 (Length) 0x01 (Channel) 0x00 (Power Level) 0x1A(OperationMode) 0x16 0xE9(Current Frequency 5865) 0xDA(CRC
int getOutSetting(byte setting) {
  byte * buf = {0x00,0x00,0xAA,0x55,0x03,0x00,0x00,0x00,0x00};
  String m = "VTX Get Settings";
	
  sa->write(buf, 9);
  log->print(m);

  byte a = -1;
    
  for(int count = 1; count < 12; count++) {
    byte b = sa->readByte();
    if( a == -1) {
      if (b == 0xAA ) {
        a = count;
      }
    }
    else
      if(setting == 0 && count - a == 4){
        return b;
      }

      if (setting == 1 && count - a == 6) {
        byte c = sa->readByte();

        return b * 0x100 + c;
      }

      if (setting == 0 && count - a == 2) {
        if(b == 0x01) {
          return 1;
        }

        if(b == 0x09) {
          return 2;
        }

        return -1;
      }
    }

    return -1;
  }
}

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

// set the channel in the range 0-40
// example: 0xAA 0x55 0x07(Command 3) 0x01(Length) 0x00(All 40 Channels 0-40) 0xB8(CRC8
void setOutChannel(byte channel) {
  byte* b =  {0x00,0x00,0xAA,0x55,0x07,0x01,channel,0x00,0x00};
  String m = "VTX Channel: ${channel}"; 
  sa->write(b, 9);
  log->print(m);
    
  int x = getOutSetting(0) ;
  String y = "VTX Actual Channel: ${x}";
  log->print(y);
}



void setup()
{
    Serial.begin(115200);
    sa = &(saSerial);
    log = &(Serial);
    pad = &(Serial);
	
    setupPins();

    // Enable buzzer and LED for duration of setup process.
    //digitalWrite(PIN_LED, HIGH);
    //digitalWrite(PIN_BUZZER, LOW);

    setupSettings();

    StateMachine::setup();
    Receiver::setup();
    Ui::setup();

    Receiver::setActiveReceiver(Receiver::ReceiverId::A);

/*
    #ifdef USE_IR_EMITTER
        Serial.begin(9600);
    #endif
    #ifdef USE_SERIAL_OUT
        Serial.begin(250000);
    #endif
*/

    // Setup complete.
    //digitalWrite(PIN_LED, LOW);
    //digitalWrite(PIN_BUZZER, HIGH);

    //Buttons::registerChangeFunc(globalMenuButtonHandler);

    // Switch to initial state.
    StateMachine::switchState(StateMachine::State::SEARCH);
}

void setupPins() {
    //pinMode(PIN_LED, OUTPUT);
    //pinMode(PIN_BUZZER, OUTPUT);
    //pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
    //pinMode(PIN_BUTTON_MODE, INPUT_PULLUP);
    //pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
    //pinMode(PIN_BUTTON_SAVE, INPUT_PULLUP);

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

#define DATA_LENGTH    0x08
#define DATA_START     0xAA
#define DATA_END       0xBB

bool newData       = false;
uint8_t numReceived = 0;
uint8_t receivedBytes[DATA_LENGTH];

void receiveBytes(Stream* stream) 
{
    static bool recvInProgress = false;
    static uint8_t ndx = 0;

    uint8_t rb;   

    while (stream->available() > 0 && newData == false) 
    {
        rb = stream->read();
        String szrb = String(rb, HEX); szrb += " ";
        //Serial.print(szrb);

        if (recvInProgress == true) 
        {
            if (rb != DATA_END) 
            {
                receivedBytes[ndx] = rb;
                if (ndx++ >= DATA_LENGTH) ndx = DATA_LENGTH - 1;
            }
            else 
            //if (stream->available() ? stream->read() == JOYSTICK_DATA_END : false)
            {
                receivedBytes[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                numReceived = ndx;  // save the number for use when printing
                ndx = 0;
                newData = true;
            }
        }

         else if (rb == DATA_START) recvInProgress = true;
    }
}

bool showNewData() 
{
    if (newData == true) 
    {
            String m = "This came in: ";
            for (byte n = 0; n < numReceived; n++) 
            {
                m += String(receivedBytes[n], HEX);
                m += " ";
            }
            _log->println(m);
	    
        newData = false;

        return true;
    }

    return false;
}



void loop() {
    Receiver::update();
    //Buttons::update();
    StateMachine::update();
    Ui::update();
    EepromSettings.update();

    /*
    if (
        StateMachine::currentState != StateMachine::State::SCREENSAVER
        && StateMachine::currentState != StateMachine::State::BANDSCAN
        && (millis() - Buttons::lastChangeTime) >
            (SCREENSAVER_TIMEOUT * 1000)
    ) {
        StateMachine::switchState(StateMachine::State::SCREENSAVER);
    }
    */

    if(showNewData()) {
      uint8_t a = receivedBytes[0];
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

