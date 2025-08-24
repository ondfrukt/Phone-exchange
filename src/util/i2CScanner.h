#include <Wire.h>

// Snabb I2C-scanner för ESP32
// - Välj buss: Wire (I2C0) eller Wire1 (I2C1)
// - Välj pinnar och frekvens (Hz)
// - Skriver resultat till Serial
void i2cScan(TwoWire &bus, int sdaPin, int sclPin, uint32_t freqHz = 100000, uint32_t timeoutMs = 50) {
  bus.end();                        // säkra om den redan var igång
  bus.setTimeOut(timeoutMs);        // undvik långa låsningar
  bus.begin(sdaPin, sclPin, freqHz);

  Serial.println();
  Serial.printf("I2C-scan på buss=%s  SDA=%d  SCL=%d  freq=%lu Hz\n",
                (&bus == &Wire) ? "Wire" : "Wire1", sdaPin, sclPin, (unsigned long)freqHz);
  Serial.println(F("Söker adresser 0x03..0x77 ..."));

  uint8_t found = 0;
  for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
    bus.beginTransmission(addr);
    uint8_t err = bus.endTransmission();
    if (err == 0) {
      Serial.printf("  ✔ Hittad enhet på 0x%02X\n", addr);
      found++;
    } else if (err == 4) {
      // valfritt: skriv ut "unknown error"
      // Serial.printf("  ! Okänt fel på 0x%02X\n", addr);
    }
    // övriga fel (1,2,3) = NACK/ingen enhet -> ignoreras
  }

  if (found == 0) Serial.println("Inga I2C-enheter hittades.");
  else            Serial.printf("Klart: %u enhet(er) hittades.\n", found);
  Serial.println();
}