// Original code: https://www.instructables.com/Solar-Powered-WiFi-Weather-Station-V30/
// rewrite by James Hughes - KB0HHM
// jhughes1010@gmail.com

// Additional rewrite by Alfred Young
// alfredy@gmail.com

#define BUILD_VER 7.7f
#define USE_EEPROM

#define SerialMonitor

//===========================================
// Includes
//===========================================
// #include "esp_deep_sleep.h"
#include "secrets.h"
// #include <PubSubClient.h>
#include <time.h>
#include <DallasTemperature.h>
#include <OneWire.h>
// #include "Wire.h"
#include <BH1750.h>
#include <Zanshin_BME680.h>
#include "Adafruit_SI1145.h"
#include <stdarg.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <driver/rtc_io.h>
#include <esp_task_wdt.h>
#include <bsec.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <esp32fota.h>

#include <EEPROM.h>
#define LOG(fmt, ...) (Serial.printf("%09llu: " fmt "\n", GetTimestamp(), ##__VA_ARGS__))

//===========================================
// Defines
//===========================================

#define WIND_SPD_PIN 14  //reed switch based anemometer count
#define RAIN_PIN     25  //reed switch based tick counter on tip bucket
#define WIND_DIR_PIN 35  //variable voltage divider output based on varying R network with reed switches
#define VOLT_PIN     33  //voltage divider for battery monitor
#define PR_PIN       15  //photoresistor pin 
#define TEMP_PIN      4  // DS18B20 hooked up to GPIO pin 4
#define LED_BUILTIN   2  //Diagnostics using built-in LED
#define SEC 1E6          //Multiplier for uS based math
#define WDT_TIMEOUT 180 // timeout
#define REBOOT_TIMEOUT      259200  // reboot every 3 days

//===========================================
// Externs
//===========================================
extern const char* ntpServer;
extern const long  gmtOffset_sec;
extern const int   daylightOffset_sec;
extern struct tm timeinfo;
extern DallasTemperature temperatureSensor;

//===========================================
// Custom structures
//===========================================
struct sensorData
{
  float tempC = 0;
  float tempF = 0;
  float windSpeed = 0;
  double windDir = 0;
  char windCardDir[16] = "NULL";
  float rain = 0;

  float bsecRawTemp = 0;
  float bsecRawHumidity = 0;
  float bsecTemp = 0;
  float bsecPressure = 0;
  float bsecGasResistance = 0;
  float bsecHumidity = 0;
  float bsecIaq = 0; 
  float bsecIaqAccuracy = 0;
  float bsecStaticIaq = 0;
  float bsecCo2Equiv = 0;
  float bsecBreathVocEquiv = 0;
  
  float uvIndex = 0;
  float visLight = 0;
  float infLight = 0;
  float lux = 0;
  float batteryVoltage = 0;
  
  int photoResistor = 0;
  int batteryAdc = 0;
};
const uint8_t bsec_config_iaq[] = {
  #include "config/generic_33v_300s_4d/bsec_iaq.txt"
};


bsec_virtual_sensor_t sensorList[10] = {
  BSEC_OUTPUT_RAW_TEMPERATURE,
  BSEC_OUTPUT_RAW_PRESSURE,
  BSEC_OUTPUT_RAW_HUMIDITY,
  BSEC_OUTPUT_RAW_GAS,
  BSEC_OUTPUT_IAQ,
  BSEC_OUTPUT_STATIC_IAQ,
  BSEC_OUTPUT_CO2_EQUIVALENT,
  BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
  BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
  BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
};

//rainfall is stored here for historical data uses RTC
struct rainfallData
{
  unsigned int intervalRainfall;
  unsigned int hourlyRainfall[24];
  unsigned int current60MinRainfall[12];
  unsigned int hourlyCarryover;
  unsigned int priorHour;
  unsigned int minuteCarryover;
  unsigned int priorMinute;
};


//===========================================
// RTC Memory storage
//===========================================
RTC_DATA_ATTR volatile int rainTicks = 0;
RTC_DATA_ATTR int lastHour = 0;
RTC_DATA_ATTR time_t nextUpdate;
RTC_DATA_ATTR struct rainfallData rainfall;
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR unsigned int elapsedTime = 0;
//======== BSEC STUFF
RTC_DATA_ATTR uint8_t sensor_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
RTC_DATA_ATTR int64_t sensor_state_time = 0;

//===========================================
// Global instantiation
//===========================================
BH1750 lightMeter(0x23);
Bsec iaqSensor;
Adafruit_SI1145 uv = Adafruit_SI1145();
bool lowBattery = false;
bool WiFiEnable = false;
const float SEA_LEVEL_PRESSURE = 1013.25;         ///< Standard atmosphere sea level pressure
OneWire oneWire(TEMP_PIN);
DallasTemperature temperatureSensor(&oneWire);

//===========================================
// ISR Prototypes
//===========================================
void IRAM_ATTR rainTick(void);
void IRAM_ATTR windTick(void);


int64_t GetTimestamp() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}


void DumpState(const char* name, const uint8_t* state) {
  LOG("%s:", name);
  for (int i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
    Serial.printf("%02x ", state[i]);
    if (i % 16 == 15) {
      Serial.print("\n");
    }
  }
  Serial.print("\n");
}
//===========================================
// OTA:
//===========================================


esp32FOTA esp32FOTA("esp32-fota-main-weather", BUILD_VER);


//===========================================
// setup:
//===========================================

void setup()
{

  setCpuFrequencyMhz (80);
  rtc_gpio_init(GPIO_NUM_12);
  rtc_gpio_set_direction(GPIO_NUM_12, RTC_GPIO_MODE_OUTPUT_ONLY);
  rtc_gpio_set_level(GPIO_NUM_12, 1);

  // if (millis() >= REBOOT_TIMEOUT) {
  //   ESP.restart();
  // }
 
  esp32FOTA.checkURL = "http://192.168.50.105/build/build.json";


  int UpdateIntervalModified = 0;
  
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
  
  pinMode(WIND_SPD_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);  
  Serial.printf("\nWeather station - Deep sleep version.\n");
  Serial.printf("Version %d\n\n", BUILD_VER);
  BlinkLED(1);
  bootCount++;
  
  Wire.begin();
  temperatureSensor.begin();
  temperatureSensor.requestTemperatures();

  // Init UV sensor
  if (! uv.begin()) {
    Serial.println("SI1145 not found");
  } else {
    Serial.println("SI1145 Initialized");
  }

  // Init BME680 using bsec library
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  iaqSensor.setConfig(bsec_config_iaq); 
  if (sensor_state_time) {
    DumpState("setState", sensor_state);
    iaqSensor.setState(sensor_state);
    if (!CheckIAQSensor()) {
      LOG("Failed to set state!");
      return;
    } else {
      LOG("Successfully set state from %lld", sensor_state_time);
    }
  } else {
    LOG("Saved state missing");
  }

  iaqSensor.updateSubscription(sensorList, sizeof(sensorList) / sizeof(sensorList[0]), BSEC_SAMPLE_RATE_ULP);
  CheckIAQSensor();

  // Init lightmeter
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE_2)) {
    Serial.println(F("BH1750 lux meter initialized"));
  } else {
    Serial.println(F("Error initialising BH1750 lux meter"));
  }

  updateWake();
  wakeupReason();

  if (WiFiEnable)
  {

    Serial.printf("Connecting to WiFi\n");
    wifiConnect();

    Serial.println("Checking if OTA build exists");
    bool updatedNeeded = esp32FOTA.execHTTPcheck();
    if (updatedNeeded) {
      Serial.println("Update needed");
      esp32FOTA.execOTA();
    } else {
      Serial.println("Update not needed");
    }

    Serial.println("Checking sensors for updates");
    processSensorUpdates();
  }

  // ESP32 Deep Sleep Mode
  UpdateIntervalModified = nextUpdate - mktime(&timeinfo);
  if (UpdateIntervalModified < 3)
  {
    UpdateIntervalModified = 3;
  }

  esp_task_wdt_reset();
  sleepyTime(UpdateIntervalModified);
}

//===================================================
// loop: these are not the droids you are looking for
//===================================================
void loop()
{
  //no loop code
}


//===========================================================
// wakeup_reason: action based on WAKE reason
// 1. Power up
// 2. WAKE on EXT0 - increment rain tip gauge count and sleep
// 3. WAKE on TIMER - send sensor data to IOT target
//===========================================================
//check for WAKE reason and respond accordingly
void wakeupReason()
{
  esp_sleep_wakeup_cause_t wakeupReason;

  wakeupReason = esp_sleep_get_wakeup_cause();
  Serial.printf("Wakeup reason: %d\n", wakeupReason);

  switch (wakeupReason)
  {
    //Rain Tip Gauge
    case ESP_SLEEP_WAKEUP_EXT0 :
      Serial.printf("Wakeup caused by external signal using RTC_IO\n");
      WiFiEnable = false;
      rainTicks++;
      break;

    //Timer
    case ESP_SLEEP_WAKEUP_TIMER :
      Serial.printf("Wakeup caused by timer\n");
      WiFiEnable = true;
      //Rainfall interrupt pin set up
      delay(100); //possible settling time on pin to charge
      attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainTick, FALLING);
      attachInterrupt(digitalPinToInterrupt(WIND_SPD_PIN), windTick, RISING);
      break;

    //Initial boot or other default reason
    default :
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeupReason);
      WiFiEnable = true;
      break;
  }
}

// ======================
// sleepyTime: prepare for sleep and set
// timer and EXT0 WAKE events
//===========================================
void sleepyTime(long UpdateIntervalModified)
{
  Serial.println("\n\n\nGoing to sleep now...");
  Serial.printf("Waking in %i seconds\n\n\n\n\n\n\n\n\n\n", UpdateIntervalModified);

  rtc_gpio_set_level(GPIO_NUM_12, 0);
  esp_sleep_enable_timer_wakeup(UpdateIntervalModified * SEC);
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, 0);
  elapsedTime = (int)millis() / 1000;
  esp_deep_sleep_start();
}

//====================================================
// processSensorUpdates: Connect to WiFi, read sensors
// and record sensors at IOT destination and MQTT
//====================================================
void processSensorUpdates(void)
{
  struct sensorData environment;
#ifdef USE_EEPROM
  readEEPROM(&rainfall);
#endif
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  printTimeNextWake();

  //Get Sensor data
  Serial.println("read env\n");
  readSensors(&environment);

  //move rainTicks into hourly containers
  rainfall.intervalRainfall = rainTicks;
  Serial.printf("Current Hour: %i\n\n", timeinfo.tm_hour);
  addTipsToHour(rainTicks);
  clearRainfallHour(timeinfo.tm_hour + 1);
  rainTicks = 0;

  //Conditional write of rainfall data to EEPROM
#ifdef USE_EEPROM
  Serial.println("rain write\n");
  conditionalWriteEEPROM(&rainfall);
#endif
  //send sensor data to IOT destination
  Serial.println("send data\n");
  sendData(&environment);
  BlinkLED(2);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

//===========================================
// BlinkLED: Blink BUILTIN x times
//===========================================
void BlinkLED(int num)
{
  int x;
  //if reason code =0, then set num =1 (just so I can see something)
  if (!num)
  {
    num = 1;
  }
  for (x = 0; x < num; x++)
  {
    //LED ON
    digitalWrite(LED_BUILTIN, HIGH);
    delay(150);
    //LED OFF
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
  }
}
//===========================================
//  bsec / bme680 sensor check
//===========================================

bool CheckIAQSensor() {
  if (iaqSensor.status < BSEC_OK) {
    Serial.printf("BSEC error, status %d!", iaqSensor.status);
    return false;;
  } else if (iaqSensor.status > BSEC_OK) {
    Serial.printf("BSEC warning, status %d!", iaqSensor.status);
  }
  if (iaqSensor.bme680Status < BME680_OK) {
    Serial.printf("Sensor error, bme680_status %d!", iaqSensor.bme680Status);
    return false;
  } else if (iaqSensor.bme680Status > BME680_OK) {
    Serial.printf("Sensor warning, status %d!", iaqSensor.bme680Status);
  }

  return true;
}

//===========================================
// MonPrintf: diagnostic printf to terminal
//===========================================
void MonPrintf( const char* format, ... ) {
  char buffer[200];
  va_list args;
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end( args );
#ifdef SerialMonitor
  Serial.printf("%s", buffer);
#endif
}


// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}