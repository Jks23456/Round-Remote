//Based on: https://www.mschoeffler.de/2017/10/05/tutorial-how-to-use-the-gy-521-module-mpu-6050-breakout-board-with-the-arduino-uno/
#include <Wire.h>;
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>;

#define PI 3.1415926535897932384626433832795

#define ssid "WLAN31iH"
#define password "solidar20packt"

#define LED_COUNT 13
#define LED_PIN 2

//MPU6050
const int MPU_ADDR = 0x68;

const int MPU_GYRO_MODE = 0x10; //Gyro Mode 1
const double MPU_GYRO_MODE_CON = 32.8;
int MPU_GYRO_OFF[3];

const int MPU_ACC_MODE = 0x0;
const double MPU_ACC_MODE_CON = 16384;

int16_t vec_X[] = {1, 0, 0};
int16_t vec_Y[] = {0, 1, 0};
int16_t vec_Z[] = {0, 0, 1};

char tmp_str[7]; // temporary variable used in convert function

//WIFI
IPAddress server(192, 168, 2, 109);
WiFiClient client;

//Adafruit
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
const int topLED=3;
int topLED_current=0;

//Main
int orr = 0;
int content[LED_COUNT][3]; //Colors in rgb 0-255



/*----------------------------------------------------------------------------------------------------------------------------------------
                                                                              MPU6050
------------------------------------------------------------------------------------------------------------------------------------------*/
char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

void readGyro(int16_t pData[3]) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  for (int i = 0; i < 3; i++) {
    pData[i] = ((Wire.read() << 8 | Wire.read())-MPU_GYRO_OFF[i]) / MPU_GYRO_MODE_CON;
  }
}

void readAcc(int16_t pData[3]){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);
  for (int i = 0; i < 3; i++) {
    pData[i] = Wire.read() << 8 | Wire.read();
  }
}

double trackGyro(int pAxis, int pCicle){ //x=0,y=1,z=2
  int16_t data[3];
  float tmp=0;
  float retVal = 0;
  
  readGyro(data); //clear
  
  for (int i=0;i<pCicle;i++){
    readGyro(data);
    if (data[pAxis]>MPU_GYRO_MODE_CON){
      retVal += (data[pAxis]/MPU_GYRO_MODE_CON)/8000;
    }
    delayMicroseconds(115);
  }
  return double(retVal);
}

int getOrientation() { // retVal: 0 = Obj moving, 1 = Flat, 2 = stand
  int16_t vec_ACC[3];
  readAcc(vec_ACC);
  double len = getVecLength(vec_ACC);
  if (len < 17500 && len > 14000) {
    double ang = getVecAngle(vec_ACC, vec_Z);
    if (ang < 10) {
      return 1;
    } else if (ang > 80 && ang < 100) {
      return 2;
    }
  }
  return 0;
}

double getVecAngle(int16_t vec0[3], int16_t vec1[3]) {
  return acos(getVecScalar(vec0, vec1) / ( getVecLength(vec0) * getVecLength(vec1))) * 180 / PI;
}

double getVecLength(int16_t vec[3]) {
  return sqrt(pow(vec[0], 2) + pow(vec[1], 2) + pow(vec[2], 2));
}

double getVecScalar(int16_t vec0[3], int16_t vec1[3]) {
  return vec0[0] * vec1[0] + vec0[1] * vec1[1] + vec0[2] * vec1[2];
}

/*----------------------------------------------------------------------------------------------------------------------------------------
                                                                              WIFI
------------------------------------------------------------------------------------------------------------------------------------------*/
boolean connectWifi(int pTimeout){
  WiFi.begin(ssid, password);
  boolean ret = false;
  for (int i=0; i<pTimeout*100; i++){
    if (WiFi.status() == WL_CONNECTED){
      ret = true;
      break;
    }
    delay(10);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("Your ESP is connected!");
    Serial.println("Your IP address is: ");
    Serial.println(WiFi.localIP());
  }else {
    Serial.println("");
    Serial.println("WiFi not connected");
  }
  return ret;
}

boolean writeTCP(char* pMessage){
  if (client.connect(server, 5000)){
    Serial.print("Write: ");
    Serial.println(pMessage);
    client.write(pMessage);
    return true;
  }
  return false;
}
/*----------------------------------------------------------------------------------------------------------------------------------------
                                                                             Adafruit
------------------------------------------------------------------------------------------------------------------------------------------*/

void configurLED(){
  int16_t g[3];
  readAcc(g);
  for(int i=0;i<3;i++){//rotate point g by 180
    g[i] = g[i] * -1;
  }
  int v = round(getVecAngle(g,vec_Y) / (round(360/LED_COUNT)));//get Alpha and convert ° to LED´s (1 LED == 22°)
  if (g[0] > 0){ // left or right shift
    topLED_current = topLED + v;
  }else{
    topLED_current = topLED - v;
  }

  if (topLED_current < 0){
    topLED_current = LED_COUNT + topLED_current;
  }else if(topLED_current >= LED_COUNT){
    topLED_current = topLED_current - LED_COUNT;
  }
}


/*----------------------------------------------------------------------------------------------------------------------------------------
                                                                              Main
------------------------------------------------------------------------------------------------------------------------------------------*/
void fillContent(int r, int g, int b){
  for(int i=0;i<LED_COUNT;i++){
    setContent(i, r,g,b);
  }
}

void setContent(int pPixel, int r, int g, int b){
  content[pPixel][0] = r;
  content[pPixel][1] = g;
  content[pPixel][2] = b;  
}

void drawContent(){
  configurLED();
  int tmp;
  for (int i=0;i<LED_COUNT;i++){
    tmp = topLED_current + i;
    if (tmp>=LED_COUNT){
      tmp -= LED_COUNT;
    }
    strip.setPixelColor(tmp, content[i][0], content[i][1], content[i][2]);
  }
  strip.show();
}

boolean isActivated(){
  return true;
}

void switchFlat(){
  Serial.println("Orr: Flat");
  boolean capture = true;
  int cicle=800; // every 
  float angle=0;
  int16_t r[3];
  while(capture){
    cicle = cicle - 1;
    readGyro(r);
    angle = angle + r[2];
    
    
    if (cicle<=0){
      cicle=800;
      Serial.println(angle);
    }
    delayMicroseconds(115);
  }
}

void switchStand(){
  Serial.println("Orr: Stand");

  boolean capture=true;
  while(capture){
    
  }
  
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  //conf MPU6050
  Wire.beginTransmission(MPU_ADDR);//wake MPU up
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(false);//set Gyro Mode
  Wire.write(0x1B);
  Wire.write(MPU_GYRO_MODE);
  Wire.endTransmission(false);//set ACC Mode
  Wire.write(0x1C);
  Wire.write(MPU_ACC_MODE);
  Wire.endTransmission(true);

  //connect Wifi
  //connectWifi(30);

  //config Adafruit
  /*strip.begin();
  strip.show();
  strip.setBrightness(50);*/

  //config Main
  fillContent(0,0,0);
  setContent(0,255,0,0);
  setContent(1,255,0,0);
  setContent(2,255,0,0);
  setContent(3,255,0,0);
  drawContent();

  //test Gyro
  int16_t data[3];
  int16_t calc[3];
  for (int i=0;i<5;i++){
    Serial.println("Gyro");
    readGyro(data);
    for (int j=0;j<3;j++){
      calc[i] = data[i];
      Serial.println(data[j]);
    }
    Serial.println("---");
    delay(100);
  }

  for (int i=0;i<3;i++){
      MPU_GYRO_OFF[i] = calc[i]/5;
      Serial.println(data[i]);
    }

  //test Acc
  for (int i=0;i<5;i++){
    Serial.println("Acc");
    readAcc(data);
    for (int j=0;j<3;j++){
      Serial.println(data[j]);
    }
    Serial.println("---");
    delay(100);
  }
}

void loop() {
  Serial.println(trackGyro(2,1000));
  delay(100);
  /*if (isActivated()){
    configurLED();
    switch (getOrientation()){
      case 1:
        switchFlat();        
      case 2:
        switchStand();
      default:
        return;
    }
  }else{
    delay(100);
  }*/
}
