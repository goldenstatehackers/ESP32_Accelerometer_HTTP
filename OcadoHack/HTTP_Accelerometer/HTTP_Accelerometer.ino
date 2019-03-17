#include<Wire.h>
#include <HTTPClient.h>
#include <WiFi.h>

const char* ssid = "ocado-guest";
const char* pass = "noncomplex";

const int MPU6050_addr=0x68;
int16_t AccY , blue= 33, maped_data;
const long interval  = 100;
enum gap_degree{FIRST, SECOND, THIRD, CRASH};
int16_t min_value = 400;
int16_t max_value = 800;


unsigned long previousMillis = 0;        // will store last time accelerometer_data was updatet

void setup(){
  pinMode(blue, OUTPUT);
  
  Wire.begin();
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
  }
  Serial.print("Connected to wifi");
  
}

void loop(){

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
            
      read_average_data();  
      
      digitalWrite(blue, LOW);
    }

}
// 
void gap_check(int16_t accelerate_value){
  Serial.print("\n");
  if(accelerate_value > min_value + 100 ){
    Serial.print("first");
    
    //HTTP POST REQUEST
    http_post();
    //Buzzer   
    digitalWrite(blue, HIGH);
    delay(500);
    
  }
  
}
// Reads 20 values from accelerometer and takes highest one. 
void read_average_data(){
  int16_t  maximum = 0;
  
  for(int i = 0; i < 20; i++){
    get_accel_data();
    maped_data = map(AccY, 0, 20000, 0, 2000);
    if(maped_data > maximum){
      maximum = maped_data;   
    }
  }
  if(min_value < maximum || max_value < maximum){
    gap_check(maped_data);                            //Check for gap
  }
  
}
// get raw data from accelerator MPU6050
int16_t get_accel_data(){
      Wire.beginTransmission(MPU6050_addr);
      Wire.write(0x3B);
      Wire.endTransmission(false);
      Wire.requestFrom(MPU6050_addr,14,true);
      AccY=Wire.read()<<8|Wire.read();

}
void http_post(){
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
 
    HTTPClient http;   
 
    http.begin("http://jsonplaceholder.typicode.com/posts");                      //Specify destination for HTTP request
    http.addHeader("Content-Type", "text/plain");             //Specify content-type header
 
    int httpResponseCode = http.POST("HOLE");   //Send the actual POST request
 
    if(httpResponseCode> 0){
 
      String response = http.getString();                       //Get the response to the request
 
      Serial.println(httpResponseCode);   //Print return code
      Serial.println(response);           //Print request answer
 
    }else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
 
    http.end();  //Free resources
 
  }else{
 
    Serial.println("Error in WiFi connection");   
 
  }
 
  delay(1000);
}
