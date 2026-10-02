/*
 * KG200Z LoRaWAN Devboard — 01 AT Komut Terminali (ESP32 ile, otomatik baud tespiti)
 *
 * ESP32'yi USB ile Quectel KG200Z arasında köprü yapar. Açılışta modülün hangi
 * hızda (115200 / 9600) cevap verdiğini kendisi bulur ve sürüm bilgisini
 * okur; ardından Seri Monitöre yazdığınız her komutu modüle iletir.
 *
 * Bağlantı (ESP32 DevKit örneği — pinleri kendi kartınıza göre değiştirin):
 *   KG200Z TX  → ESP32 GPIO16 (MODUL_RX_PIN)
 *   KG200Z RX  → ESP32 GPIO17 (MODUL_TX_PIN)
 *   KG200Z RST → ESP32 GPIO4  (MODUL_RST_PIN, aktif-düşük, isteğe bağlı)
 *   KG200Z 3V3 → 3.3 V (en az 0.5 A),  GND → GND
 * Kart 3.3 V lojiktir; 5 V'luk bir denetleyicide TX/RX'e seviye dönüştürücü şarttır.
 * Anten takılı değilken veri göndermeyin (AT+...SEND / JOIN): RF çıkış katı zarar görebilir.
 *
 * Seri Monitör: 115200 baud, satır sonu "Both NL & CR".
 *
 * EN: USB ↔ KG200Z AT terminal on an ESP32 with automatic baud detection
 *     (115200 / 9600). Never transmit without an antenna attached.
 */

#include <Arduino.h>

// ─────────── KULLANICI AYARLARI ───────────
#define MODUL_RX_PIN   16     // ESP32 RX ← KG200Z TX
#define MODUL_TX_PIN   17     // ESP32 TX → KG200Z RX
#define MODUL_RST_PIN  4      // ESP32 → KG200Z RST (bağlı değilse -1 yapın)
// ──────────────────────────────────────────

HardwareSerial modul(2);
const uint32_t HIZLAR[] = {115200, 9600};

String komut(const char* k, uint32_t sure = 1000) {
  while (modul.available()) modul.read();
  modul.print(k); modul.print("\r\n");
  String c;
  uint32_t t = millis();
  while (millis() - t < sure) {
    while (modul.available()) c += (char)modul.read();
    if (c.indexOf("OK") >= 0 || c.indexOf("ERROR") >= 0) break;
  }
  c.trim();
  return c;
}

uint32_t hizBul() {
  for (uint32_t hiz : HIZLAR) {
    modul.updateBaudRate(hiz);
    delay(50);
    for (int i = 0; i < 3; i++) {
      if (komut("AT").indexOf("OK") >= 0) return hiz;
    }
  }
  return 0;
}

void setup() {
  Serial.begin(115200);
  modul.begin(HIZLAR[0], SERIAL_8N1, MODUL_RX_PIN, MODUL_TX_PIN);
  delay(300);
  Serial.println("\n=== KG200Z AT terminali ===");

  if (MODUL_RST_PIN >= 0) {                  // modülü temiz başlat
    pinMode(MODUL_RST_PIN, OUTPUT);
    digitalWrite(MODUL_RST_PIN, LOW);  delay(100);
    digitalWrite(MODUL_RST_PIN, HIGH); delay(1500);
  }

  uint32_t hiz = hizBul();
  if (!hiz) {
    Serial.println("[HATA] Modul 115200 ve 9600 baud'da cevap vermedi.");
    Serial.println("       TX/RX capraz mi, GND ortak mi, 3V3 beslemesi var mi?");
  } else {
    Serial.printf("Modul %lu baud'da cevap veriyor.\n", (unsigned long)hiz);
    Serial.println("ATI      -> " + komut("ATI"));
    Serial.println("AT+CGMR  -> " + komut("AT+CGMR"));
  }
  Serial.println("Komut yazabilirsiniz.");
}

void loop() {
  while (Serial.available()) modul.write(Serial.read());
  while (modul.available()) Serial.write(modul.read());
}
