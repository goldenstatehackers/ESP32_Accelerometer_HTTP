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

/**@brief Check for interrupt from the accelerometer
 */
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

/**@brief Reads 20 values from accelerometer and takes highest one. 
 */
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

/**@brief get raw data from accelerometer MPU6050
 */
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



/* ************************************************* GPS SIMULATION */

/**@brief Date and Time structure. */
typedef struct
{
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hours;
    uint8_t  minutes;
    uint8_t  seconds;
} date_time_t;


/**@brief   Location and Speed data structure. */
struct lns_loc_speed_s
{
    bool                            utc_time_time_present;                     /**< UTC Time present (0=not present, 1=present). */
   
    uint16_t                        instant_speed;                             /**< Instantaneous Speed (km/h). */

    int32_t                         latitude;                                  /**< Latitude (10e-7 degrees). */
    int32_t                         longitude;                                 /**< Longitude (10e-7 degrees). */

    date_time_t                 utc_time;                                  /**< UTC Time. */
};


lns_loc_speed_s m_sim_location_speed = 
{
    .utc_time_time_present = true,
  
    .instant_speed           = 60,         // = 60 km/h

    .latitude                = -103123567, // = -10.3123567 degrees
    .longitude               = 601234567,  // = 60.1234567 degrees

    .utc_time                = {
                                 .year    = 2019,
                                 .month   = 3,
                                 .day     = 17,
                                 .hours   = 11,
                                 .minutes = 23,
                                 .seconds = 33
                               }
};

void date_time_print()
{
  Serial.print (m_sim_location_speed.utc_time.year);
  Serial.print("\n");
  Serial.print (m_sim_location_speed.utc_time.month);
  Serial.print("\n");
  Serial.print (m_sim_location_speed.utc_time.day);
  Serial.print("\n");
  Serial.print (m_sim_location_speed.utc_time.hours);
  Serial.print("\n");
  Serial.print (m_sim_location_speed.utc_time.minutes);
  Serial.print("\n");
  Serial.print (m_sim_location_speed.utc_time.seconds);
  Serial.print("\n");
}

void loc_simulation_print()
{
   Serial.print(m_sim_location_speed.latitude);
   Serial.print("\n");
   Serial.print(m_sim_location_speed.longitude);
   Serial.print("\n");
   date_time_print();
}

/**@brief Provide simulated location and speed.
 */
void loc_speed_simulation_update(void)
{
    m_sim_location_speed.latitude++;
    m_sim_location_speed.longitude++;

    increment_time(&m_sim_location_speed.utc_time);
}



void increment_time(date_time_t * p_time)
{
    p_time->seconds++;
    if (p_time->seconds > 59)
    {
        p_time->seconds = 0;
        p_time->minutes++;
        if (p_time->minutes > 59)
        {
            p_time->minutes = 0;
            p_time->hours++;
            if (p_time->hours > 24)
            {
                p_time->hours = 0;
                p_time->day++;
                if (p_time->day > 31)
                {
                    p_time->day = 0;
                    p_time->month++;
                    if (p_time->month > 12)
                    {
                        p_time->year++;
                    }
                }
            }
        }
    }
}

