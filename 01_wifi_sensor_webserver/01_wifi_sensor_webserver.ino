/*
 * [예제 1] Wi-Fi 방식 - ESP32-S3 센서 웹서버
 * PUMPKIN IDEA FACTORY MAKERSPACE 통신팀
 *
 * 동작: ESP32-S3가 스스로 Wi-Fi 공유기(AP)가 되고,
 *       스마트폰/노트북이 접속해 브라우저(192.168.4.1)에서 센서값을 실시간으로 봅니다.
 *
 * 연결:  DHT22 DATA -> GPIO4   (VCC 3.3V, GND)
 *        조도센서(CdS 분압) -> GPIO1 (ADC)
 * 라이브러리: "DHT sensor library" (Adafruit), "Adafruit Unified Sensor"
 */
#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHT_PIN   4
#define DHT_TYPE  DHT22      // DHT11을 쓰면 DHT11로 변경
#define LIGHT_PIN 1

const char* AP_SSID = "PIF-SENSOR";   // 접속할 Wi-Fi 이름
const char* AP_PASS = "12345678";     // 8자 이상

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

float temperature = NAN, humidity = NAN;
int   lightValue  = 0;
unsigned long lastRead = 0;

void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;     // 읽기 실패 시 이전 값 유지
  if (!isnan(h)) humidity    = h;
  lightValue = analogRead(LIGHT_PIN); // 0~4095
}

// 브라우저에 보여줄 화면 (2초마다 /data 를 받아와 갱신)
const char PAGE[] = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PIF 센서 모니터</title>
<style>
 body{font-family:sans-serif;background:#111;color:#eee;text-align:center}
 .card{display:inline-block;margin:10px;padding:20px 30px;border-radius:12px;background:#222}
 .v{font-size:2.4em;color:#ffa31a}
</style></head><body>
<h2>PUMPKIN IDEA FACTORY MAKERSPACE<br>Wi-Fi 센서 모니터</h2>
<div class="card">온도<div class="v" id="t">-</div>°C</div>
<div class="card">습도<div class="v" id="h">-</div>%</div>
<div class="card">조도<div class="v" id="l">-</div>/4095</div>
<script>
async function up(){
  try{
    const r = await fetch('/data'); const d = await r.json();
    document.getElementById('t').textContent = d.t;
    document.getElementById('h').textContent = d.h;
    document.getElementById('l').textContent = d.l;
  }catch(e){}
}
setInterval(up, 2000); up();
</script></body></html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", PAGE);
}

void handleData() {
  String json = "{\"t\":" + String(temperature, 1) +
                ",\"h\":" + String(humidity, 1) +
                ",\"l\":" + String(lightValue) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  analogReadResolution(12);

  // AP 모드: 공유기 없이 ESP32-S3가 직접 Wi-Fi를 만듭니다.
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("Wi-Fi 이름: "); Serial.println(AP_SSID);
  Serial.print("접속 주소: http://"); Serial.println(WiFi.softAPIP()); // 보통 192.168.4.1

  /* 집/학교 공유기에 붙이고 싶다면 위 3줄 대신:
     WiFi.mode(WIFI_STA);
     WiFi.begin("공유기이름", "비밀번호");
     while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
     Serial.println(WiFi.localIP());
  */

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  if (millis() - lastRead > 2000) {   // DHT22는 2초 간격 이상으로 읽기
    lastRead = millis();
    readSensors();
    Serial.printf("T=%.1f H=%.1f L=%d\n", temperature, humidity, lightValue);
  }
}
