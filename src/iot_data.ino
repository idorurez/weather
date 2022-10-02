#include <WiFi.h>
#include <MySQL_Generic.h>


#define MYSQL_DEBUG_PORT      Serial
// Debug Level from 0 to 4
#define _MYSQL_LOGLEVEL_      1

void wifiConnect(void)
{
  Serial.println("Connecting to " + String(ssid));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while ( WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(100);
  } 
}

void sendData(struct sensorData *environment)
{
  int hourPtr = timeinfo.tm_hour;
  IPAddress server(192, 168, 50, 105);
  uint16_t server_port = db_port;
  Serial.printf("Version %f\n\n", BUILD_VER);
  char query[1024];
  char INSERT_DATA[] = "INSERT INTO weatherdb.master_sensor_vals ( DS18B20_TEMP, WIND_DIR, WIND_CARD_DIR, WIND_SPEED, RAIN, SI1145_UV_INDEX, SI1145_VIS_LIGHT, SI1145_INF_LIGHT, BSEC_RAW_TEMP, BSEC_RAW_HUMIDITY, BSEC_TEMP, BSEC_HUMIDITY, BSEC_PRESSURE, BSEC_GAS_RESISTANCE, BSEC_IAQ, BSEC_IAQ_ACCURACY, BSEC_STATIC_IAQ, BSEC_CO2_EQUIV, BSEC_BREATH_VOC_EQUIV, BH1750_LUX, BATTERY_VOLTAGE, BUILD_VER ) VALUES (%.3f, %.3f, \"%s\", %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f)";
  // char INSERT_DATA[] = "INSERT INTO weatherdb.master_sensor_vals ( DS18B20_TEMP, WIND_DIR, WIND_CARD_DIR, WIND_SPEED, RAIN, SI1145_UV_INDEX, SI1145_VIS_LIGHT, SI1145_INF_LIGHT, BSEC_RAW_TEMP, BSEC_RAW_HUMIDITY, BSEC_PRESSURE, BSEC_GAS_RESISTANCE, BH1750_LUX, BATTERY_VOLTAGE, BUILD_VER ) VALUES (%.3f, %.3f, \"%s\", %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f)";
  sprintf(query, INSERT_DATA, environment->tempF, 
                              environment->windDir, 
                              String(environment->windCardDir), 
                              environment->windSpeed, 
                              rainfall.hourlyRainfall[hourPtr] * 0.011, 
                              environment->uvIndex, 
                              environment->visLight, 
                              environment->infLight, 
                              environment->bsecRawTemp, 
                              environment->bsecRawHumidity, 
                              environment->bsecTemp, 
                              environment->bsecHumidity, 
                              environment->bsecPressure, 
                              environment->bsecGasResistance, 
                              environment->bsecIaq, 
                              environment->bsecIaqAccuracy, 
                              environment->bsecStaticIaq, 
                              environment->bsecCo2Equiv, 
                              environment->bsecBreathVocEquiv, 
                              environment->lux, 
                              environment->batteryVoltage,
                              BUILD_VER);

  WiFiClient client;
  MySQL_Connection conn((Client *)&client);

  MYSQL_DISPLAY(MYSQL_MARIADB_GENERIC_VERSION);
  MySQL_Query query_mem = MySQL_Query(&conn);
  MYSQL_DISPLAY("Connecting...");

  //if (conn.connect(server, server_port, user, password))
  if (conn.connectNonBlocking(server, db_port, db_user, db_pwd) != RESULT_FAIL) {
    
    if (conn.connected()) {
        Serial.println(query);
      // Execute the query
      // KH, check if valid before fetching
      if (!query_mem.execute(query)) {
        MYSQL_DISPLAY("Insert error");
      } else {
        MYSQL_DISPLAY("Data Inserted.");
      }
    } else {
      MYSQL_DISPLAY("Disconnected from Server. Can't insert.");
    } 
  } else {
    MYSQL_DISPLAY("\nConnect failed. Trying again on next iteration.");
  }

  conn.close();                     // close the connection
}

