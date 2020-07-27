#include <Arduino.h>
#include <TM1637Display.h>

// Module connection pins (Digital Pins)
#define CLK 22
#define DIO 23

// The amount of time (in milliseconds) between tests
#define TEST_DELAY   2000
#include <SoftwareSerial.h>

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
  };

TM1637Display display(CLK, DIO);

void setup()
{
  Serial.begin(115200);
  delay(10);
  
  Serial.println();
}

void loop()
{
  int k;
  uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
  uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };
  Serial.println(F("setBrightness"));
  display.setBrightness(0x0f);

  Serial.println(F("All segments on"));
  // All segments on
  display.setSegments(data);
  delay(TEST_DELAY);

  Serial.println(F("Selectively set different digits"));
  // Selectively set different digits
  data[0] = display.encodeDigit(0);
  data[1] = display.encodeDigit(1);
  data[2] = display.encodeDigit(2);
  data[3] = display.encodeDigit(3);
  display.setSegments(data);
  delay(TEST_DELAY);

  /*
  for(k = 3; k >= 0; k--) {
  display.setSegments(data, 1, k);
  delay(TEST_DELAY);
  }
  */

  display.clear();
  display.setSegments(data+2, 2, 2);
  delay(TEST_DELAY);

  display.clear();
  display.setSegments(data+2, 2, 1);
  delay(TEST_DELAY);

  display.clear();
  display.setSegments(data+1, 3, 1);
  delay(TEST_DELAY);


  Serial.println(F("Вывод точки00"));
    delay(TEST_DELAY);
    delay(TEST_DELAY);
    delay(TEST_DELAY);
    delay(TEST_DELAY);
    delay(TEST_DELAY);
  // Run through all the dots
  for(k=0; k <= 4; k++) {
    display.showNumberDecEx(0, (0x80 >> k), true);
    delay(TEST_DELAY);
  }
  k = 0;
    Serial.println(F("к=0"));
  display.showNumberDecEx(0, (0x80 >> k), true);
      delay(TEST_DELAY);
  k = 1;
      Serial.println(F("к=1"));
  display.showNumberDecEx(0, (0x80 >> k), true);
      delay(TEST_DELAY);
  k = 2;
      Serial.println(F("к=2"));
  display.showNumberDecEx(0, (0x80 >> k), true);
      delay(TEST_DELAY);
  k = 3;
      Serial.println(F("к=3"));
  display.showNumberDecEx(0, (0x80 >> k), true);
      delay(TEST_DELAY);


  // Brightness Test
  for(k = 0; k < 4; k++)
  data[k] = 0xff;
  for(k = 0; k < 7; k++) {
    display.setBrightness(k);
    display.setSegments(data);
    delay(TEST_DELAY);
  }
  
  // On/Off test
  for(k = 0; k < 4; k++) {
    display.setBrightness(7, false);  // Turn off
    display.setSegments(data);
    delay(TEST_DELAY);
    display.setBrightness(7, true); // Turn on
    display.setSegments(data);
    delay(TEST_DELAY);  
  }

 
  // Done!
  display.setSegments(SEG_DONE);

  while(1);
}
