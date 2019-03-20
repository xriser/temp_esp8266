#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp8266.h>
#include "Timer.h"
#include "MQ135.h" 
#include <Wire.h>
#include "HTU21D.h"


#include <ESP8266WiFiMulti.h>

ESP8266WiFiMulti wifiMulti;

Timer tm;

const char *INFLUXDB_HOST = "10.0.0.7";
const uint16_t INFLUXDB_PORT = 8086;

const char *DATABASE = "power";
const char *DB_USER = "power";
const char *DB_PASSWORD = "pow123212";

//Create an instance of the object
HTU21D myHumidity;
MQ135 gasSensor = MQ135(A0);


// ==================================== VARS ==================================
int LED = D4; // Wemos build in led

int val;// Defines a numeric variable val
int count = 0;

float ppm, rzero, temp, humd, at[10], st = 0;
int p = 0, t = 0, h = 0, sppm = 0, sh = 0;
int appm[10], ah[10];


#define LED_ON   digitalWrite(LED, 1)
#define LED_OFF  digitalWrite(LED, 0)


const char* ssid = "x1";
const char* password = "pass";

const char *mqtt_server = "10.0.0.7"; 
const int mqtt_port = 1883; 
const char *mqtt_user = "user"; 
const char *mqtt_pass = "password";

WiFiClient espClient;
PubSubClient client(espClient);


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println();
}



void setup() {

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("SSID", "password");
  wifiMulti.addAP("Hubber_2G", "password");
  wifiMulti.addAP("HubberHub_2G", "password");

  pinMode(LED, OUTPUT); // Definition LED as output interface
  
  // start serial port
  Serial.begin(115200);

  setup_wifi();

  ArduinoOTA.setHostname("test_222");
      
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
 
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Ready");

  int tickEvent = tm.every(2000, blynkLed);

  tm.every(1000, getPpm);
  tm.every(1000, getTemp);
  tm.every(1000, getHumidity);

  myHumidity.begin();
  
}

// ================================================= END SETUP ======================================


void reconnect() {
  // Loop until we're reconnected
  
     Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266_TEST-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user, mqtt_pass)) {
      Serial.println("connected");
 
      client.subscribe("test/test");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  
}



void setup_wifi() {

  delay(500);
  Serial.println();
  
  Serial.println("Connecting Wifi...");
  
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

}

int sort_desc(const void *cmp1, const void *cmp2)
{
  // Need to cast the void * to int *
  int a = *((int *)cmp1);
  int b = *((int *)cmp2);
  // The comparison
  return a > b ? -1 : (a < b ? 1 : 0);
  // A simpler, probably faster way:
  //return b - a;
}


void blynkLed() {
   
  val = digitalRead(LED); // The digital interface is assigned a value of 3 to read val

  if (val == LOW)  {
    LED_ON;
  } else {
    LED_OFF;
    }
  
}

void getTemp() {
  
  temp = myHumidity.readTemperature();

  at[t]=temp;
  t++;

  if (t > 9) {
    int at_length = sizeof(at) / sizeof(at[0]);
    qsort(at, at_length, sizeof(at[0]), sort_desc);
    st = at[t/2];
    st = round(st*10)/10.0;

    char buffer[7];
    dtostrf(st, 1, 2, buffer);
    //mqtt_client.publish("room/air/gy21_temp", buffer, true);

    t = 0;
    Serial.println("");
    Serial.print("Temperature: ");
    Serial.print(st, 2);
    Serial.println("C");
 
  }
}


void getHumidity() {
  humd = myHumidity.readHumidity();

  ah[h]=humd;
  h++;

  if (h > 9) {
    int ah_length = sizeof(ah) / sizeof(ah[0]);
    qsort(ah, ah_length, sizeof(ah[0]), sort_desc);
    sh = ah[h/2];

    char buffer[7];
    dtostrf(sh, 1, 0, buffer);
   //mqtt_client.publish("room/air/gy21_humd", buffer, true);
    
    h = 0;
    
    Serial.println("");
    Serial.print("Humidity: ");
    Serial.print(sh, 1);
    Serial.println("%");
  }
 }


void getPpm() {
  ppm = gasSensor.getCorrectedPPM(temp, humd);

  appm[p]=ppm;
  p++;

  if (p > 9) {
    int appm_length = sizeof(appm) / sizeof(appm[0]);
    qsort(appm, appm_length, sizeof(appm[0]), sort_desc);
    sppm = appm[p/2];

    char buffer[7];
    dtostrf(sppm, 1, 0, buffer);

    //mqtt_client.publish("room/air/mq135", buffer, true);

    p = 0;
    Serial.println("");
    Serial.print("ppm: ");
    Serial.println(sppm);
  
  }
 }  



void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
   }
   ArduinoOTA.handle();

 tm.update();
 delay(10);
}
