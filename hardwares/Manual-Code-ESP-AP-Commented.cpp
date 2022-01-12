// Khai báo các thư viện sẽ sử dụng
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <Stepper.h>
#include <ESPAsyncWebServer.h>

Stepper my_Stepper(200, D1, D3, D2, D4);

const char *ssid = "ESP8266-Access-Point";
const char *password = "123456789";

const char *PARAM_INPUT_1 = "output";
const char *PARAM_INPUT_2 = "state";

int Direction = 0;
int Rotation = 0;

// Tạo Server trên cổng 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Helvetica; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP8266 Asynchronous Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

String outputState(int output)
{
  if (digitalRead(output))
  {
    return "checked";
  }
  else
  {
    return "";
  }
}

// Thay thế %BUTTONPLACEHOLDER% bằng những nút bấm 
String processor(const String &var)
{
  Serial.println(var);
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    buttons += "<h4>Up</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"5\" " + outputState(5) + "><span class=\"slider\"></span></label>";
    buttons += "<h4>Down</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

void Stepper1(int Direction, int Rotation)
{ // Hàm điều khiển động cơ bước
  for (int i = 0; i < Rotation; i++)
  {                                   
    my_Stepper.step(Direction * 200); // 200 = 360 độ
  }
}

void setup()
{
  // Mở cổng Serial
  Serial.begin(115200);

  pinMode(D0, INPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, INPUT_PULLUP);
  pinMode(D7, INPUT_PULLUP);
  pinMode(A0, INPUT);
  my_Stepper.setSpeed(200);

  Serial.print("Setting AP (Access Point)…");
  // Thiết lập ESP8266 ở dạng một điểm truy cập
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // In địa chỉ IP của ESP
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  // Gửi một GET request đến <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String inputMessage1;
              String inputMessage2;
              // Nhận giá trị từ các biến có trong GET request tại <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
              if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2))
              {
                inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
                inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
                digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
                Direction = inputMessage1.toInt();
                Rotation = inputMessage2.toInt();
              }
              else
              {
                inputMessage1 = "No message sent";
                inputMessage2 = "No message sent";
              }
              Serial.print("GPIO: ");
              Serial.print(Direction);
              Serial.print(" - Set to: ");
              Serial.println(Rotation);
              request->send(200, "text/plain", "OK");
            });

  // Bắt đầu Server
  server.begin();
}

void loop()
{
  if (Direction == 5)
  {
    Stepper1(-1, 1);
  }
  else if (Direction == 4)
  {
    Stepper1(1, 1);
  }
  else if (Rotation == 0)
  {
    Stepper1(0, 0);
  }
}