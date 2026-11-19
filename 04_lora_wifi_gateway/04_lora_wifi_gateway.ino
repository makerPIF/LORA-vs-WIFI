/*
 * [예제 4] LoRa + Wi-Fi 게이트웨이 (두 방식을 합친 실전 구조)
 * PUMPKIN IDEA FACTORY MAKERSPACE 통신팀
 *
 *   [센서노드 N1, N2...] --LoRa(멀리, 저전력)--> [ESP32-S3 게이트웨이] --Wi-Fi(가까이, 빠름)--> [스마트폰 브라우저]
 *
 * 동작: LoRa로 여러 노드의 데이터를 받아 저장하고,
 *       Wi-Fi AP(PIF-GATEWAY)에 접속한 스마트폰에서 http://192.168.4.1 로 노드별 현황을 봅니다.
 *
 * 배선/LoRa 설정은 예제 2와 동일. 송신 노드는 예제 2를 NODE_ID만 바꿔 여러 대 사용.
 */
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebServer.h>

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

const char* AP_SSID = "PIF-GATEWAY";
const char* AP_PASS = "12345678";

#define MAX_NODES 8
struct NodeData {
  char  id[16];
  unsigned long count;
  float t, h;
  int   l, rssi;
  float snr;
  unsigned long lastMs;   // 마지막 수신 시각
  bool  used;
};
NodeData nodes[MAX_NODES];

WebServer server(80);

NodeData* findNode(const char* id) {
  for (int i = 0; i < MAX_NODES; i++)
    if (nodes[i].used && strcmp(nodes[i].id, id) == 0) return &nodes[i];
  for (int i = 0; i < MAX_NODES; i++)
    if (!nodes[i].used) { nodes[i].used = true; strncpy(nodes[i].id, id, 15); nodes[i].id[15] = 0; return &nodes[i]; }
  return nullptr;   // 노드가 너무 많음
}

const char PAGE[] = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PIF LoRa 게이트웨이</title>
<style>
 body{font-family:sans-serif;background:#111;color:#eee;margin:16px}
 table{border-collapse:collapse;width:100%}
 th,td{padding:8px;border-bottom:1px solid #333;text-align:center}
 th{color:#ffa31a}.old{color:#777}
</style></head><body>
<h3>PUMPKIN IDEA FACTORY MAKERSPACE · LoRa 게이트웨이</h3>
<table><thead><tr><th>노드</th><th>온도°C</th><th>습도%</th><th>조도</th><th>RSSI</th><th>SNR</th><th>수신</th></tr></thead>
<tbody id="tb"><tr><td colspan="7">수신 대기 중...</td></tr></tbody></table>
<script>
async function up(){
  try{
    const d = await (await fetch('/data')).json();
    if(!d.length) return;
    document.getElementById('tb').innerHTML = d.map(n =>
      `<tr class="${n.ago>30?'old':''}"><td>${n.id}</td><td>${n.t}</td><td>${n.h}</td><td>${n.l}</td>
       <td>${n.rssi} dBm</td><td>${n.snr} dB</td><td>${n.ago}초 전</td></tr>`).join('');
  }catch(e){}
}
setInterval(up, 2000); up();
</script></body></html>
)rawliteral";

void handleRoot() { server.send(200, "text/html; charset=utf-8", PAGE); }

void handleData() {
  String json = "[";
  bool first = true;
  for (int i = 0; i < MAX_NODES; i++) {
    if (!nodes[i].used) continue;
    NodeData& n = nodes[i];
    if (!first) json += ",";
    first = false;
    json += "{\"id\":\"" + String(n.id) + "\",\"t\":" + String(n.t, 1) +
            ",\"h\":" + String(n.h, 1) + ",\"l\":" + String(n.l) +
            ",\"rssi\":" + String(n.rssi) + ",\"snr\":" + String(n.snr, 1) +
            ",\"ago\":" + String((millis() - n.lastMs) / 1000) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void checkLoRa() {
  int packetSize = LoRa.parsePacket();
  if (packetSize == 0) return;

  char buf[128];
  int n = 0;
  while (LoRa.available() && n < (int)sizeof(buf) - 1) buf[n++] = (char)LoRa.read();
  buf[n] = '\0';

  char id[16]; unsigned long cnt; float t, h; int l;
  if (sscanf(buf, "PIF,%15[^,],%lu,%f,%f,%d", id, &cnt, &t, &h, &l) != 6) return;

  NodeData* nd = findNode(id);
  if (!nd) return;
  nd->count = cnt; nd->t = t; nd->h = h; nd->l = l;
  nd->rssi = LoRa.packetRssi();
  nd->snr  = LoRa.packetSnr();
  nd->lastMs = millis();
  Serial.printf("수신 %s #%lu  RSSI %d\n", id, cnt, nd->rssi);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(SPI);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa 초기화 실패!");
    while (true) delay(1000);
  }
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SYNC);
  LoRa.enableCrc();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("게이트웨이 주소: http://"); Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  checkLoRa();            // LoRa 패킷 확인
  server.handleClient();  // 웹 요청 처리
}
