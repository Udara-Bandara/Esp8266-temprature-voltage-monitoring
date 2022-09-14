/* S- Configurations *********************************************************/
#define BLYNK_TEMPLATE_ID "USE_YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_DEVICE_NAME "USE_YOUR_BLYNK_DEVICE_NAME"
#define BLYNK_AUTH_TOKEN "USE_YOUR_BLYNK_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#define WIFI_SSID "USE_YOUR_WIFI_SSID"
#define WIFI_PASSWORD "USE_YOUR_WIFI_PASSWORD"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = WIFI_SSID; 
char pass[] = WIFI_PASSWORD; 

// inverter on voltage and cutoff voltage
float on_voltage = 12;
float off_voltage = 11.5;

// refresh interval
uint32_t delayMS = 1000;
uint32_t delayWifi = 2000;

// inverter virtual pin
int pinValue1; 

// 0-25v voltage sensor is connected with the analog pin A0 of the nodemcu
int vSensor_1 = A0; 
int switch_1 = 0;

//For 0-25v voltage sensor
float correctionFactor = -0.14;
//float correctionFactor = 0.50;

float vOut = 0.0;
float vIn = 0.0;

// two resistors 30K and 7.5k ohm
float R1 = 30000;  //
float R2 = 7500; //
int value = 0;

String inverterStatus = "UNKNOWN";

int maxTemperature = 55;
bool isAutoMode = false;

#define DHTPIN_1 2      // Temperature sensor is connected with the digital pin D4 of the nodemcu
#define DHTPIN_2 5      // Temperature sensor is connected with the digital pin D1 of the nodemcu
#define DHTTYPE DHT11     // DHT 11  

DHT dht_1(DHTPIN_1, DHTTYPE);
DHT dht_2(DHTPIN_2, DHTTYPE);

BlynkTimer timer;

BLYNK_WRITE(V5)
{
  if (param.asInt() == 1) {
    isAutoMode = true;
    Blynk.virtualWrite(V6, 1);
    Serial.println("Inverter Auto Mode OFF!");
  } else {
    isAutoMode = false;
    Blynk.virtualWrite(V6, 0);
    Serial.println("Inverter Auto Mode ON!");
  }
}

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V5); 
}

// Temperature Monitor
void temperatureMonitor() {
  float h_1 = dht_1.readHumidity();
  float t_1 = dht_1.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  float h_2 = dht_2.readHumidity();
  float t_2 = dht_2.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

  if (isnan(h_1) || isnan(t_1)) {
    Blynk.virtualWrite(V0, 0);
    Blynk.virtualWrite(V1, 0);
    Serial.println("Error reading temperature in sensor DHT 1!");
  }

  if (isnan(h_2) || isnan(t_2)) {
    Blynk.virtualWrite(V2, 0);
    Blynk.virtualWrite(V3, 0);
    Serial.println("Error reading temperature in sensor DHT 2!");
  }

  if (isnan(t_1) && isnan(t_2)) {
    return;
  }

  if ((t_2 > maxTemperature || t_1 > maxTemperature) && isAutoMode == true) {
    digitalWrite(switch_1, LOW);
    inverterStatus = "POWER OFF";
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    Serial.println("Temprature is high, Inverter is going to Power OFF!!");
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  } else {
    if (isAutoMode == true) {
      inverterStatus = "POWER ON";
    } else {
      inverterStatus = "DEFAULT MODE";
    }
    setInverterStatus();
  }
  
  Blynk.virtualWrite(V0, h_1);
  Blynk.virtualWrite(V1, t_1);

  Blynk.virtualWrite(V2, t_2);
  Blynk.virtualWrite(V3, h_2);
  
  Blynk.virtualWrite(V7, inverterStatus);

  // serial print -> inverter status 
  Serial.print("Inverter -->");
  Serial.println(inverterStatus);
  Serial.println("----------------------------------------------------------------------------------");
  // serial print -> DHT 11 Sensor 1 status
  Serial.print("DHT11 Sensor 1 --> Temperature : ");
  Serial.print(t_1);
  Serial.print("    Humidity : ");
  Serial.println(h_1);
  // serial print -> DHT 11 Sensor 2 status
  Serial.println("----------------------------------------------------------------------------------");
  Serial.print("DHT11 Sensor 2 --> Temperature : ");
  Serial.print(t_2);
  Serial.print("    Humidity : ");
  Serial.println(h_2);
  Serial.println("----------------------------------------------------------------------------------");

}

// Voltage Monitor
void voltageMonitor() {
  float vTotal = 0.0;
  int maxIndex = 10; // number of samples

  // loop multiple times and get average reading
  for (int i=0; i < maxIndex; i++) {
    vTotal = vTotal + analogRead(vSensor_1);
  }
  value = vTotal/maxIndex;

  // voltage calculation
  vOut = (value * 3.3) / 1024.0; // 3.3V
  vIn = vOut / (R2/(R1+R2));

  vIn = vIn + correctionFactor; 

  setInverterStatus();
  
  Serial.print("Voltage Sensor 1 --> Voltage : "); 
  Serial.print(vIn, 4);
  Serial.println("V");
  Blynk.virtualWrite(V4, vIn); // send voltage value to blynk
  Serial.println("----------------------------------------------------------------------------------");
}


// check battery voltage & update inverter status
void setInverterStatus() {
    if (vIn >= on_voltage) {
    inverterStatus = "POWER ON";
    digitalWrite(switch_1, HIGH);
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    Serial.println("Inverter is Power ON.");
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  } else {
    inverterStatus = "POWER OFF";
    digitalWrite(switch_1, LOW);
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    Serial.println("Battery is low, Inverter is going to Power OFF!!");
    Serial.println("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  }
  Blynk.virtualWrite(V7, inverterStatus);
}

// connect to network
void connectNetwork() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Network scaning start...");
    int availableNetworks = WiFi.scanNetworks();
    
    if (availableNetworks > 0){
      Serial.printf("%d network(s) found.\n", availableNetworks);
  
      for (int i = 0; i < availableNetworks; i++)
      {
        if(WiFi.SSID(i) == WIFI_SSID) {
          Serial.printf("%s wifi network founded.\n", WIFI_SSID);
          Blynk.begin(auth, ssid, pass);
          Serial.printf("Connected to wifi network %s.\n", WIFI_SSID);
        } 
      }
      if (WiFi.status() != WL_CONNECTED) {
          Serial.printf("%s wifi network not founded.\n", WIFI_SSID);
      }
    } else {
      Serial.println("Wifi network not founded.");
    }
  }
}

void setup() {
  //  pin configuration  
  pinMode(switch_1, OUTPUT);
  pinMode(vSensor_1, INPUT); 
  // set default value 
  digitalWrite(switch_1, LOW);
  // voltage monitor  
  voltageMonitor();
  
  Serial.begin(115200);

  dht_1.begin();
  dht_2.begin();
    
  // Temperature Monitor
  timer.setInterval(2500L, temperatureMonitor);
  // Voltage Monitor
  timer.setInterval(delayMS, voltageMonitor);
  // connect network
  timer.setInterval(delayWifi, connectNetwork);
}

void loop() {
  Blynk.run();
  timer.run();
}
