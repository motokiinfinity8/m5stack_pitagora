#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h> 
#include <Wire.h>
#include <VL53L0X.h>
#include <VL6180X.h>
#include <Adafruit_NeoPixel.h>
#include "esp_system.h"
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "AudioGeneratorWAV.h"

// initial value for Wifi
const char* ssid     = "**************";
const char* password = "*************";
const char* host = "192.168.0.13";    //リモコン制御用Raspberry
WiFiServer server(80);
IPAddress ip(192, 168, 100, 52); //固定IP 192.168.0.52
IPAddress gateway(192,168, 100, 0);
IPAddress subnet(255, 255, 255, 0);
IPAddress DNS(192, 168, 100, 0);

// initial value for audio
AudioGeneratorWAV *wav;
AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;
bool g_audio_play;

#define SerialPort Serial

#define NEOPIXEL_PIN     5
#define M_SENSE_RST_PIN     16
#define F_SENSE_RST_PIN     17
#define PIXEL_COUNT  40

VL53L0X m_sensor;
VL6180X f_sensor;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);



unsigned int m_sensor_val, f_sensor_val;
int g_state, g_action;
bool g_state_flg, g_finish_flg, g_display_mode;

/////////////////////////////
// 音楽再生タスク処理
/////////////////////////////
/**
void playWAV(void *pvParameters) {
  while(1){
    Serial.print("wav Task!");
    bool audio_play = g_audio_play;
    if(audio_play == true){
        Serial.print("wav Play");
        if (wav->isRunning()) {
          if (!wav->loop()){
            wav->stop();
            //g_audio_play = false;
          }
        }
      }
    }
}
**/


/////////////////////////////
// LED表示タスク処理
/////////////////////////////
/**void DisplayLED(void *pvParameters) {
  int g_pos, g_pos_old;
  
  while(1){
    if(m_sensor_val < 35 || m_sensor_val > 1000) g_pos = -1;
    else  g_pos = (m_sensor_val-35) / 10.5;

    if(g_pos >= PIXEL_COUNT) g_pos = PIXEL_COUNT;
    int i;  
    if( g_pos !=  g_pos_old){
       for(i=0; i<PIXEL_COUNT; i++){
         switch(g_state){
         case 0: 
           strip.setPixelColor(i, strip.Color(0, 0, 0));
           break;
         case 1:
           if(i < g_pos)  strip.setPixelColor(i, Wheel(i * 255 / PIXEL_COUNT));
           else strip.setPixelColor(i, strip.Color(0, 0, 0));
           break;
         case 2: 
           //strip.setPixelColor(i, strip.Color(0, 0, 0));
           break;
         case 3: 
           strip.setPixelColor(i, Wheel(i * 255 / PIXEL_COUNT));
           break;           
       }
      }
      //delay(1);
      //portDISABLE_INTERRUPTS();
      //delay(1);
      strip.show();
      //portENABLE_INTERRUPTS();
    }
    g_pos_old = g_pos;
  }
}
**/

void setup() 
{
  M5.begin();
  Wire.begin();
  pinMode(NEOPIXEL_PIN, OUTPUT);
  pinMode(M_SENSE_RST_PIN, OUTPUT);
  pinMode(F_SENSE_RST_PIN, OUTPUT);
  digitalWrite(M_SENSE_RST_PIN, LOW);
  digitalWrite(F_SENSE_RST_PIN, LOW);

  strip.begin();
  strip.setBrightness(64);
  delay(1);  //安定待ち
  strip.show(); // Initialize all pixels to 'off'
  g_state = 0;  // 0: STANBY  1: 中段  2: 下段
  g_action = 0;
  g_state_flg = 1;
  g_finish_flg = 0;
  g_display_mode = 0;
  
  // センサー初期化設定
  digitalWrite(M_SENSE_RST_PIN, HIGH);
  m_sensor.init();
  delay(100);
  m_sensor.setAddress(24);
  m_sensor.setTimeout(500);
  m_sensor.startContinuous();  

  digitalWrite(F_SENSE_RST_PIN, HIGH);
  f_sensor.init();
  delay(100);
  f_sensor.setAddress(25);
  f_sensor.configureDefault();
  f_sensor.setTimeout(500);
  //f_sensor.startRangeContinuous(10);  

  //WAV再生設定
  file = new AudioFileSourceSD("/pita6.wav");
  out = new AudioOutputI2S(0,1); 
  out->SetOutputModeMono(true);
  out->SetGain(0.5); 
  wav = new AudioGeneratorWAV(); 
  
  // Start device display with ID of m_sensor
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE ,BLACK); // Set pixel color; 1 on the monochrome screen
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0,0); M5.Lcd.print("VL53L0X");


  // WiFi設定
  // We start by connecting to a WiFi network
  /**Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.config(ip,gateway,subnet,DNS);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  M5.Lcd.println("WiFi connected.");
  Serial.println("IP address: ");
  M5.Lcd.println("IP address: ");
  Serial.println(WiFi.localIP()); 
  M5.Lcd.println(WiFi.localIP());
*/
  

  rainbowCycle(2, 0, PIXEL_COUNT-1);
  
//  TaskHandle_t th_wav, th_led; //マルチタスクハンドル定義
//  xTaskCreatePinnedToCore(playWAV, "playWAV", 4096, NULL, 5, &th_wav, 0); //マルチタスク起動
//  xTaskCreatePinnedToCore(DisplayLED, "DisplayLED", 4096, NULL, 7, &th_led, 0); //マルチタスク起動
    
  SerialPort.begin(115200);

}


void loop() 
{
  // Use WiFiClient class to create TCP connections
  //WiFiClient client;
  //const int httpPort = 1880;  //Node-REDの使用ポート
  //if (!client.connect(host, httpPort)) {
  //  Serial.println("connection failed");
  //  return;
 // }
  HTTPClient http;
  
  unsigned int m_sensor_tmp[5];
  unsigned int f_sensor_tmp[5];

  // センサー測定値誤差ばらつきの平均化
  m_sensor_tmp[0] = m_sensor.readRangeContinuousMillimeters();
  f_sensor_tmp[0] = f_sensor.readRangeSingleMillimeters();
  delay(5);

  m_sensor_tmp[1] = m_sensor.readRangeContinuousMillimeters();
  f_sensor_tmp[1] = f_sensor.readRangeSingleMillimeters();
  delay(5);
  
  m_sensor_tmp[2] = m_sensor.readRangeContinuousMillimeters();
  f_sensor_tmp[2] = f_sensor.readRangeSingleMillimeters();
  delay(5);

  unsigned int m_sensor_val = (m_sensor_tmp[0] +m_sensor_tmp[1]+m_sensor_tmp[2])/3;    
  unsigned int f_sensor_val = (f_sensor_tmp[0] +f_sensor_tmp[1]+f_sensor_tmp[2])/3;     
  //unsigned int m_sensor_val = m_sensor.readRangeContinuousMillimeters();
  //unsigned int f_sensor_val = f_sensor.readRangeSingleMillimeters();

   // ボタンが押された場合、display_modeを反転
   M5.update();
   if(M5.BtnB.wasPressed()){
      delay(5);
      g_display_mode =!g_display_mode;
      if(g_display_mode==1) M5.Lcd.fillScreen(BLACK);   
    }


  // 状態管理
  switch(g_state){
    //*****************//
    // STANBYモード時
    //*****************//
    // 中段センサーでボール検出 or 中段ボタン押しで State=1に遷移
    case 0:
      // STATE遷移時初期処理
      if(g_state_flg == 1){
        Serial.println("State 0 : STAND-BY");
        // 音楽停止
        //wav->stop();
        g_audio_play = false;
        //String body = "pitagora_sound=0";
        // client.print(String("GET ") + body +" HTTP/1.1\r\n" +
        //       "Host: " + host +":1880"+ "\r\n" + 
        //       "Connection: close\r\n\r\n");
        http.begin("http://192.168.0.13:1880/no_sound");
        // LCD映像固定
        M5.Lcd.fillScreen(BLACK);
        M5.Lcd.setTextColor(GREEN ,BLACK);  
      }

      // 中段センサーの値が15cm以下を検出した場合、状態1に遷移
      if(m_sensor_val < 150){
        g_action = 1;
      }
      break;
    //*****************//
    // 中段コロコロモード時
    //*****************//
    case 1:
      // STATE遷移時初期処理
      if(g_state_flg == 1){
        Serial.println("State 1 : Middle-Line");
        // ピタゴラ音楽再生
        //file = new AudioFileSourceSD("/pitagora.wav");
        //wav = new AudioGeneratorWAV(); 
        //wav->begin(file, out);
        //g_audio_play = true;
        // String body = "pitagora_sound=1";
        // client.print(String("GET ") + body +" HTTP/1.1\r\n" +
        //       "Host: " + host +":1880"+ "\r\n" + 
        //       "Connection: close\r\n\r\n");
        http.begin("http://192.168.0.13:1880/pitagora_sound");
        // LCD映像固定
        M5.Lcd.drawJpgFile(SD, "/bisuke.jpg");
      }

      // STAETE定常処理      
      // 音楽再生
      // 中段センサーの値が48cm以上になった場合、状態2に遷移
      if(m_sensor_val > 450){
        g_action = 2;
      }

      // 一定時間経過した場合、状態0に遷移
      
      break;
      
    //*****************//
    // 最下段コロコロモード時
    //*****************//    
    case 2:
      // STATE遷移時初期処理
      if(g_state_flg == 1){
         Serial.println("State 2 : Under-Line");
      }

       Serial.println(f_sensor_val);
      // STAETE定常処理      
      // 下段センサーの値が0cmになった場合、10cm以上に代わる事を確認してから状態3に遷移
      if(f_sensor_val < 80){
       g_finish_flg = 1;
      }
      if(g_finish_flg ==1 && f_sensor_val == 255){
         g_action = 3;
      }

      // 一定時間経過した場合、状態0に遷移
      
      break;

    //*****************//
    // ピタゴラフィニッシュ
    //*****************//    
    case 3:
      // STATE遷移時初期処理
      if(g_state_flg == 1){
        Serial.println("State 3 : Pitagora-Finish");
        g_finish_flg = 0;
        // 音楽停止
        //String body = "pitagora_sound=0";
        // client.print(String("GET ") + body +" HTTP/1.1\r\n" +
        //       "Host: " + host +":1880"+ "\r\n" + 
        //       "Connection: close\r\n\r\n");
        http.begin("http://192.168.0.13:1880/no_sound");
        g_audio_play = false;
        // LCD映像固定
        M5.Lcd.fillScreen(BLACK);    
      }
      
      delay(1000);
      // ピタゴラ映像再生
      M5.Lcd.drawJpgFile(SD, "/pitagora.jpg");
      // 音楽再生
      file = new AudioFileSourceSD("/pita_finish.wav");
      wav = new AudioGeneratorWAV(); 
      wav->begin(file, out);
      while(wav->isRunning()){
       if (!wav->loop()) wav->stop();
      }
      delay(2000);
      M5.Lcd.fillScreen(BLACK);   
      g_action = 0;
      break;
    
  }

  
  // LCD Display
  //M5.Lcd.setTextColor(GREEN ,BLACK);
  if(g_display_mode ==1){
    M5.Lcd.setTextSize(3);
    M5.Lcd.setCursor(0,    0); M5.Lcd.print("State:");
    M5.Lcd.setCursor(128,  0); M5.Lcd.print(g_state);
    M5.Lcd.setCursor(0,  48); M5.Lcd.print("Dist:");
    M5.Lcd.setCursor(128,  48); M5.Lcd.print("          ");
    M5.Lcd.setCursor(128,  48); M5.Lcd.print(m_sensor_val);
    M5.Lcd.setCursor(128,  96); M5.Lcd.print("          ");
    M5.Lcd.setCursor(128,  96); M5.Lcd.print(f_sensor_val);
  //M5.Lcd.setCursor(128, 128); M5.Lcd.print(g_pos_old);
  }
  
  // LED Display
  // LED位置換算(実測による補正付)  25個まで(390mm以下)で条件分け
  int g_pos, g_pos_old;
  if(m_sensor_val < 40 || m_sensor_val > 1000) g_pos = -1;
  // 390mm以下の場合
  else if(m_sensor_val<= 390){
    g_pos = (m_sensor_val-40) / 14;
  }// 390mm以上の場合
  else  g_pos = 25 + (m_sensor_val-390) / 4.5;

  if(g_pos >= PIXEL_COUNT) g_pos = PIXEL_COUNT;

  
  int i;
  switch(g_state){
    case 0: 
      if(g_state_flg == 1){
          for(i=0; i<PIXEL_COUNT; i++){
            strip.setPixelColor(i, strip.Color(0, 0, 0));
          }
      }
      break;
    case 1:
    case 2: 
      for(i=0; i<PIXEL_COUNT; i++){
         if(i < g_pos)  strip.setPixelColor(i, Wheel(i * 255 / PIXEL_COUNT));
         else strip.setPixelColor(i, strip.Color(0, 0, 0));
       }
       break;
    case 3: 
       strip.setPixelColor(i, Wheel(i * 255 / PIXEL_COUNT));
       break;           
    }
    delay(1);
    portDISABLE_INTERRUPTS();
    delay(1);
    strip.show();
    portENABLE_INTERRUPTS();
  g_pos_old = g_pos;
    
  // STATE遷移処理(毎回処理)
  if(g_action != g_state){
      g_state_flg = 1;
      g_state = g_action;
  }else{
     g_state_flg = 0;
  }
  M5.update();
}


void rainbowCycle(int SpeedDelay,  int start_led, int end_led) {
  uint16_t i, j;
  
  set_led_out_of_range(start_led, end_led);

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for (int i=start_led; i<= end_led; i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / (end_led-start_led)) + j) & 255));
    }
    portDISABLE_INTERRUPTS();
    strip.show();
    portENABLE_INTERRUPTS();
    delay(SpeedDelay);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void set_led_out_of_range(int start_led, int end_led){
  int i;
  
  for(i=0; i<start_led ;i++){
     strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  if(end_led < strip.numPixels()){
    for(i=(end_led+1) ; i<strip.numPixels();i++){
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  delay(5);  //安定待ち
    portDISABLE_INTERRUPTS();
    strip.show();
    portENABLE_INTERRUPTS();
}
