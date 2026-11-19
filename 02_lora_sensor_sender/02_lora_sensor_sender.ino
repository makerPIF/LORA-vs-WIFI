/*
 * [예제 2] LoRa 방식 - 센서 노드(송신기)
 * PUMPKIN IDEA FACTORY MAKERSPACE 통신팀
 *
 * 동작: 센서값을 읽어 5초마다 LoRa 무선으로 한 줄짜리 문자열 패킷을 보냅니다.
 *       패킷 형식:  PIF,노드ID,순번,온도,습도,조도   예) PIF,N1,37,23.4,51.2,1880
 *
 * LoRa 모듈: SX1276/SX1278 계열 (RFM95W, Ra-01H 등 920MHz 대역 모듈)
 * 배선(ESP32-S3 <-> LoRa 모듈)
 *   SCK  GPIO12 | MISO GPIO13 | MOSI GPIO11 | NSS GPIO10 | RST GPIO9 | DIO0 GPIO8
 *   3.3V, GND  (※ LoRa 모듈은 3.3V 전용, 5V 금지)
 * 센서: DHT22 DATA -> GPIO4, 조도(CdS 분압) -> GPIO1
 * 라이브러리: "LoRa" (Sandeep Mistry), "DHT sensor library" (Adafruit)
 *
 * ※ 안테나를 연결하지 않고 송신하면 모듈이 손상될 수 있습니다.
 */
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>

// ---- LoRa 핀 ----
#define LORA_SCK  12
#define LORA_MISO 13
#define LORA_MOSI 11
#define LORA_SS   10
#define LORA_RST  9
#define LORA_DIO0 8

// ---- 센서 핀 ----
#define DHT_PIN   4
#define DHT_TYPE  DHT22
#define LIGHT_PIN 1

// ---- LoRa 설정 (송신기·수신기가 반드시 똑같아야 통신됨) ----
#define LORA_FREQ  922.1E6   // 국내 LoRa 대역(920.9~923.3MHz) 안의 채널
#define LORA_SF    9         // 확산계수 7~12 : 클수록 멀리 가지만 느려짐
#define LORA_BW    125E3     // 대역폭
#define LORA_CR    5         // 코딩레이트 4/5
#define LORA_SYNC  0x2A      // 우리 팀 전용 "암호" (다른 팀 패킷 걸러내기)
#define LORA_POWER 10        // 송신 출력 dBm (실습은 낮게, 법정 출력 이내로)

const char* NODE_ID = "N1";  // 노드가 여러 대면 N2, N3 ... 로 바꿔서 업로드

DHT dht(DHT_PIN, DHT_TYPE);
unsigned long counter = 0;
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  dht.begin();
  analogReadResolution(12);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa 초기화 실패! 배선/전원/주파수를 확인하세요.");
    while (true) delay(1000);
  }
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SYNC);
  LoRa.setTxPower(LORA_POWER);
  LoRa.enableCrc();             // 깨진 패킷 자동 검출

  Serial.println("LoRa 송신기 준비 완료");
}

void loop() {
  if (millis() - lastSend < 5000) return;   // 5초 간격 (LoRa는 '가끔, 짧게' 보내는 통신)
  lastSend = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int   l = analogRead(LIGHT_PIN);
  if (isnan(t)) t = -99.0;                  // 센서 오류 표시값
  if (isnan(h)) h = -99.0;

  char msg[64];
  snprintf(msg, sizeof(msg), "PIF,%s,%lu,%.1f,%.1f,%d", NODE_ID, counter, t, h, l);

  unsigned long t0 = millis();
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();                         // 전송이 끝날 때까지 기다림
  unsigned long airTime = millis() - t0;

  Serial.printf("송신: %s  (%d바이트, 전송시간 약 %lums)\n", msg, (int)strlen(msg), airTime);
  counter++;
}
