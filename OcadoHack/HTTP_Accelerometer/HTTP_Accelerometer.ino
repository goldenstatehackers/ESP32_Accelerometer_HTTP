#include <Wire.h>
#include <HTTPClient.h>
#include <WiFi.h>

/** 
 *
 * @brief Application for monitoring the road quality, developed for the purposes of the 
 *        Ocado Technology hackaton, Hack for innovation.
 *        
 * @ref https://www.meetup.com/Ocado-Technology-Events-Sofia/events/258728041/
 *
 * The application uses MPU6050 6-axis accelerometer and gyroscope for monitoring the road quality,
 * GPS module to get the coordinates of potholes and road bumps
 * and HTTP requests to a server to send the data.
 * 
 * The application generates an interrupt when the car goes over a pothole and sends the location 
 * coordinates and the level of the pothole to the server. 
 * Potholes are classified on 4 degrees.
 * 
 * @note The GPS module is not used in this revision. Instead, GPS coordinates are simulated for 
 *       testing purposes.
 */

const char* ssid = "ocado-guest";
const char* pass = "noncomplex";

const int MPU6050_addr = 0x68;
int16_t AccY;                            // Variable to store raw accelerometer readings from Y axis.
int16_t buzzer = 33;                     // ESP32 digital pin number for the buzzer
int16_t maped_data;                      // Data from the Y axis, mapped to a different interval.
const long interval  = 100;

enum gap_degree{ FIRST, SECOND, THIRD, CRASH }; /* Pothole classification */

int16_t min_value = 400;
int16_t max_value = 800;

unsigned long previousMillis = 0;        // will store last time accelerometer_data was updated

void setup()
{
  pinMode(buzzer, OUTPUT); // Buzzer notification on event

  // Configure accelerometer
  Wire.begin();
  Wire.beginTransmission(MPU6050_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(115200);

  // Connect to the wifi network
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }
  Serial.print("Connected to wifi");
  
}

void loop()
{

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
            
      read_average_data();  // Read accelerometer data
      
      digitalWrite(buzzer, LOW);
    }

}

// Check for interrupt from the accelerometer
void gap_check(int16_t accelerate_value)
{
  Serial.print("\n");
  
  if(accelerate_value > min_value + 100 )            // This is the treshold level, used for the interrupt generation
  {
    Serial.print("first");
    
    // Send HTTP request
    http_post();
    
    // Ring the buzzer   
    digitalWrite(buzzer, HIGH);
    delay(500);
    
  }
  
}

// Reads 20 values from accelerometer and takes highest one. 
void read_average_data()
{
  int16_t  maximum = 0;
  
  for(int i = 0; i < 20; i++)
  {
    get_accel_data();
    maped_data = map(AccY, 0, 20000, 0, 2000);
    if(maped_data > maximum)
    {
      maximum = maped_data;   
    }
  }
  
  if(min_value < maximum || max_value < maximum)
  {
    gap_check(maped_data);    // Check for interrupt
  }
  
}

// get raw data from accelerometer MPU6050
int16_t get_accel_data()
{
      Wire.beginTransmission(MPU6050_addr);
      Wire.write(0x3B);
      Wire.endTransmission(false);
      Wire.requestFrom(MPU6050_addr,14,true);
      
      AccY=Wire.read()<<8|Wire.read();
}

void http_post()
{
  //Check WiFi connection status
  if(WiFi.status() == WL_CONNECTED)
  {   
 
    HTTPClient http;   
 
    http.begin("http://jsonplaceholder.typicode.com/posts");  //Specify destination for HTTP request
    http.addHeader("Content-Type", "text/plain");             //Specify content-type header
 
    int httpResponseCode = http.POST("HOLE");                 //Send the actual POST request
 
    if(httpResponseCode> 0)
    {
 
      String response = http.getString();                      //Get the response to the request
 
      Serial.println(httpResponseCode);                       //Print return code
      Serial.println(response);                               //Print request answer
 
    }
    else 
    {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
 
    http.end();  //Free the resources
  }
  else
  {
    Serial.println("Error in WiFi connection");   
  }
 
  delay(1000);
}
