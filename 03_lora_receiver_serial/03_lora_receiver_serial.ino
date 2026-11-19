/*
 * [예제 3] LoRa 방식 - 수신기(기지국) : 시리얼 모니터로 확인
 * PUMPKIN IDEA FACTORY MAKERSPACE 통신팀
 *
 * 동작: 예제 2가 보낸 패킷을 받아 값을 분해하고,
 *       신호세기(RSSI), 신호대잡음비(SNR), 놓친 패킷 수까지 함께 출력합니다.
 *       → 거리를 늘려가며 RSSI/SNR 변화를 기록하는 '통신거리 실험'에 사용
 *
 * 배선/라이브러리/LoRa 설정은 예제 2와 동일 (센서는 필요 없음)
 */
#include <SPI.h>
#include <LoRa.h>

#define LORA_SCK  12
#define LORA_MISO 13
#define LORA_MOSI 11
#define LORA_SS   10
#define LORA_RST  9
#define LORA_DIO0 8

#define LORA_FREQ  922.1E6
#define LORA_SF    9
#define LORA_BW    125E3
#define LORA_CR    5
#define LORA_SYNC  0x2A

long lastCounter = -1;
unsigned long received = 0, lost = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

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
  LoRa.enableCrc();

  Serial.println("LoRa 수신기 대기 중...");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize == 0) return;

  char buf[128];
  int n = 0;
  while (LoRa.available() && n < (int)sizeof(buf) - 1) buf[n++] = (char)LoRa.read();
  buf[n] = '\0';

  int   rssi = LoRa.packetRssi();   // dBm, 0에 가까울수록 강함 (-120 근처면 한계)
  float snr  = LoRa.packetSnr();    // dB, 음수여도 LoRa는 수신 가능

  // 패킷 분해: PIF,N1,37,23.4,51.2,1880
  char node[16];
  unsigned long cnt;
  float t, h;
  int l;
  if (sscanf(buf, "PIF,%15[^,],%lu,%f,%f,%d", node, &cnt, &t, &h, &l) != 6) {
    Serial.printf("[무시] 형식이 다른 패킷: %s\n", buf);
    return;
  }

  // 순번으로 놓친 패킷 계산 (노드 1대 기준)
  if (lastCounter >= 0 && (long)cnt > lastCounter + 1) lost += cnt - lastCounter - 1;
  lastCounter = cnt;
  received++;
  float lossRate = 100.0f * lost / (received + lost);

  Serial.printf("[%s #%lu] 온도 %.1f°C | 습도 %.1f%% | 조도 %d || RSSI %d dBm, SNR %.1f dB, 손실 %.1f%%\n",
                node, cnt, t, h, l, rssi, snr, lossRate);
}
