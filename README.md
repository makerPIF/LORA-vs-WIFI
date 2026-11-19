# ESP32-S3 센서 모니터링 — Wi-Fi vs LoRa 예제
PUMPKIN IDEA FACTORY MAKERSPACE 통신팀

| 폴더 | 내용 | 필요한 보드 |
|---|---|---|
| 01_wifi_sensor_webserver | Wi-Fi AP + 웹페이지로 센서값 보기 | ESP32-S3 1대 + 센서 |
| 02_lora_sensor_sender | 센서값을 LoRa로 5초마다 송신 | ESP32-S3 + LoRa 모듈 + 센서 |
| 03_lora_receiver_serial | LoRa 수신 → 시리얼 모니터 (RSSI/SNR/손실률) | ESP32-S3 + LoRa 모듈 |
| 04_lora_wifi_gateway | LoRa 수신 → Wi-Fi 웹페이지 (여러 노드) | ESP32-S3 + LoRa 모듈 |

## 준비
- Arduino IDE 보드: **ESP32S3 Dev Module** (esp32 by Espressif 코어)
- 라이브러리 관리자에서 설치
  - `LoRa` (Sandeep Mistry) — SX1276/77/78 계열용
  - `DHT sensor library` (Adafruit) + `Adafruit Unified Sensor`
- LoRa 모듈: **920MHz 대역** SX1276 계열 (RFM95W 등). 433MHz 모듈(Ra-02 등)은 국내 LoRa 대역이 아님
  - SX1262 모듈(Heltec V3 등)을 쓰면 `RadioLib` 라이브러리로 바꿔야 함

## LoRa 모듈 배선
| LoRa | ESP32-S3 |
|---|---|
| SCK | GPIO12 |
| MISO | GPIO13 |
| MOSI | GPIO11 |
| NSS(CS) | GPIO10 |
| RESET | GPIO9 |
| DIO0 | GPIO8 |
| 3.3V / GND | 3.3V / GND |

센서: DHT22 DATA → GPIO4, 조도센서(CdS + 10kΩ 분압) → GPIO1

## 주의
- **안테나 없이 송신 금지** (모듈 손상)
- 송신기·수신기의 주파수, SF, BW, CR, SyncWord가 모두 같아야 통신됨
- 송신 출력과 송신 간격은 국내 전파 규정(920MHz 대역 무선설비 기준) 안에서 사용

## 실험 아이디어
1. 예제 2+3으로 거리를 50m씩 늘리며 RSSI/SNR/손실률 기록 → SF 7, 9, 12로 바꿔 비교
2. 같은 장소에서 예제 1(Wi-Fi)이 끊기는 거리와 LoRa가 끊기는 거리 비교
3. 송신기의 "전송시간(ms)" 출력을 SF별로 비교 → 멀리 가는 대가는 '느림'
