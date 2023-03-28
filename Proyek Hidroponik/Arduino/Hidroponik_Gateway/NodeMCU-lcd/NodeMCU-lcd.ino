#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {

Serial.begin(115200);
Wire.begin(5, 4);
lcd.begin(16,2);
lcd.backlight();

lcd.home();

lcd.setCursor(0, 0);
lcd.print("Halo");
lcd.setCursor(0, 1);
lcd.print("Batu");
}

void loop() {

}
