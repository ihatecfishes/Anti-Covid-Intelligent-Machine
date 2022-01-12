
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <WiFiManager.h>
#include <Stepper.h>

//Khai báo thư viện để xem thông tin khi tạo các token.
#include <addons/TokenHelper.h>

//Thư viện được sử dụng để in ra các thông tin từ một RTDB (Real time database) payload
#include <addons/RTDBHelper.h>

Stepper my_Stepper(200, D1, D3, D2, D4);


/* Khái báo API key */
#define API_KEY "AIzaSyAKdB8p-d4R26M2ZP4B3xodTtCPoduOhas"

/*  Khai báo RTDB URL */
#define DATABASE_URL "https://esp-firebase-demo-8cb7b-default-rtdb.asia-southeast1.firebasedatabase.app/" 

/* Khai báo email và mật khẩu của người dùng trên Firebase */
#define USER_EMAIL "nghiepquy6@gmail.com"
#define USER_PASSWORD "12345678"

//Khai báo Firebase Data objects
FirebaseData fbdo;
FirebaseData stream;

FirebaseAuth auth;
FirebaseConfig config;

int PostitionSwitch = digitalRead(D6);
int data = 111;

bool Initial = false;
bool PrevSwitch = false;

bool Delay = false;
unsigned long LastDelay = 0;

int Direction = 2;
int Rotation = 2;

unsigned long sendDataPrevMillis1;
int transistor;

int takeResult(FirebaseData &data)
{
  transistor = data.to<int>();
  return transistor;
}

void setup()
{

  Serial.begin(115200);
  Serial.println();
  Serial.println();
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
  // Smart Config cho Wifi
  WiFiManager wifiManager;
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    wifiManager.autoConnect("Wing's WiFi Manager");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);


  /* Gán API key (bắc buộc)*/
  config.api_key = API_KEY;

  /* Gán thông tin đăng nhập (bắc buộc) */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Gán RTDB URL (bắt buộc) */
  config.database_url = DATABASE_URL;

  /* Gán hàm callback cho việc tạo các token */
  config.token_status_callback = tokenStatusCallback; 

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  if (!Firebase.RTDB.beginStream(&stream, "/test/stream/data"))
    Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());


  //Thời gian chờ kết nối lại khi mất Wifi (10 giây - 5 phút).
  config.timeout.wifiReconnect = 10 * 1000;

  // Thời gian chờ sau mỗi lần nhận dữ liệu (1 giây - 1 phút).
  config.timeout.socketConnection = 30 * 1000;

  config.timeout.sslHandshake = 2 * 60 * 1000;

  // Thời gian chờ đến khi có phản hồi từ server (1 giây - 1 phút).
  config.timeout.serverResponse = 10 * 1000;

  //Thời gian chờ khi không có kết nối với bất kì server Firebase nào (20 giây - 2 phú)
  config.timeout.rtdbKeepAlive = 45 * 1000;

  //Thời gian chờ nếu tắt data stream trước khi tiếp tục data stream (1 giây - 1 phút)
  config.timeout.rtdbStreamReconnect = 1 * 1000;

  //Thời gian chờ mỗi khi xảy ra lỗi với readStream (3 giây - 30 giây)
  config.timeout.rtdbStreamError = 3 * 1000;

}

void Stepper1(int Direction, int Rotation)
{ // Hàm điều khiển động cơ bước
  for (int i = 0; i < Rotation; i++)
  {                                   
    my_Stepper.step(Direction * 200); // 200 = 360 độ
  }
}

void loop()
{
  int PostitionSwitch = digitalRead(D6);

  int Active = digitalRead(D7);
  digitalWrite(D5, HIGH);

  //Stream dữ liệu từ Firebase

  if (!Firebase.ready())
    return;


  if (!Firebase.RTDB.readStream(&stream))
    Serial.printf("sream read error, %s\n\n", stream.errorReason().c_str());

  if (stream.streamTimeout())
  {
    Serial.println("stream timed out, resuming...\n");

    if (!stream.httpConnected())
      Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
  }

  if (stream.streamAvailable())
  {
    Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                  stream.streamPath().c_str(),
                  stream.dataPath().c_str(),
                  stream.dataType().c_str(),
                  stream.eventType().c_str());
    printResult(stream); 
    Serial.println(takeResult(stream));
    data = takeResult(stream);
    Serial.println(data);
    Serial.println();

    //In ra kích thước của Payload
    Serial.printf("Received stream payload size: %d (Max. %d)\n\n", stream.payloadLength(), stream.maxPayloadLength());
  }
  if (Initial)
  {
    if (Active == 0)
    {
      if (PostitionSwitch == 0)
      {
        PrevSwitch = !PrevSwitch;
        Delay = true;
      }
      if (PrevSwitch)
      {
        if (Delay)
        {
          Stepper1(-1, 3);
        }
        Delay = false;
        if (data != 111)
        {
          if (data == 101)
          {
            Stepper1(1, 1);
          }
          else if (data == 110)
          {
            Stepper1(-1, 1);
          }
          else if (data == 100)
          {
            Stepper1(0, 0);
          }
        }
        else
        {
          Stepper1(-1, 1);
        }
      }
      else
      {
        if (Delay)
        {
          Stepper1(1, 3);
        }
        Delay = false;
        if (data != 111)
        {
          if (data == 101)
          {
            Stepper1(1, 1);
          }
          else if (data == 110)
          {
            Stepper1(-1, 1);
          }
          else if (data == 100)
          {
            Stepper1(0, 0);
          }
        }
        else
        {
          Stepper1(1, 1);
        }
      }
    }
  }
  else
  {
    if (PostitionSwitch == 0)
    {
      Initial = true;
    }
    else
    {
      Stepper1(1, 1);
    }
  }
}