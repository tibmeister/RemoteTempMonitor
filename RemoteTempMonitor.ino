#include <InfluxDbClient.h>
#include <max6675.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#define DEVICE "ESP8266"

#ifndef STASSID
#define STASSID "EJHome"
#define STAPSK "g2YJyfoPPoE7UIZq"
//#define STASSID "EJIoT"
//#define STAPSK "ZXGDcu%;xAZj/=+@nG&Wv:Hu"
#endif

#ifndef HOSTNAME
#define HOSTNAME "TempMonitor-"
#endif

//This is for the InfluxDB Client
#define INFLUXDB_URL "http://10.27.200.50:8186"
#define INFLUXDB_DB_NAME "environmental"
#define TZ_INFO "CST6CDT"
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nis.gov"

//Define pins for the MAX6675
#define thermoSO 4 //D2
#define thermoCS 5 //D1
#define thermoCLK 14 //D5

const char* ssid = STASSID;
const char* password = STAPSK;

MAX6675 ts(thermoCLK, thermoCS, thermoSO);

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);
Point sensor("temperature");

ESP8266WiFiMulti wifiMulti;

String hostname;

int iterations = 0;

void setup()
{
  Serial.begin(115200);

  //Set the hostname
  hostname = HOSTNAME;
  hostname += String(ESP.getChipId(), HEX);
  Serial.println("Hostname: " + hostname);

  //Start WiFi
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  
  wifiMulti.addAP(ssid, password);

  while (wifiMulti.run() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

    //Setup Influx Tags
  sensor.addTag("location","MUS");
  sensor.addTag("monitorId",hostname);
  sensor.addTag("devicemonitored","Kitchen Fridge");
  //sensor.addTag("devicemonitored","Test");

  timeSync(TZ_INFO, NTP_SERVER1, NTP_SERVER2);

  // Check server connection
  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  
  runner();

  //Pause for 20 seconds so we can do an upload if needbe
  delay(20);
  
  //Deep sleep for 220 seconds
  Serial.println("Going to sleep now");
  ESP.deepSleep(1.2e8);
}

void loop()
{
  //runner();
  //delay(10000);
}

void runner()
{
  //Get the data into a point for upload to Influx
  sensor.clearFields();
  //sensor.setTime(time(nullptr));
  sensor.addField("tempC",ts.readCelsius());
  sensor.addField("tempF",ts.readFahrenheit());

  //Write the point to the client
  client.writePoint(sensor);

  //Flush the data into influxdb
  Serial.println("Flushing data into InfluxDB");
  if (!client.flushBuffer()) {
    Serial.print("InfluxDB flush failed: ");
    Serial.print(client.getLastStatusCode());
    Serial.print(" : ");
    Serial.println(client.getLastErrorMessage());
    Serial.print("Full buffer: ");
    Serial.println(client.isBufferFull() ? "Yes" : "No");
  }
  
  //Let the MAX6674 settle for atleast 250ms
  delay(250);
  // basic readout test, just print the current temp
  //Serial.print(timeClient.getFormattedTime());
  //Serial.print(" UTC - ");
  Serial.print(ts.readCelsius(), 2);
  Serial.print(" C / ");
  Serial.print(ts.readFahrenheit(), 2);
  Serial.print(" F ");
  Serial.println();
}
