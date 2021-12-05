//=============================================================
//Variables for wifi server setup and api keys for IOT
//Constants for WAKE frequency and UOM for sensors
//=============================================================

//===========================================
//Controls supression of the MonPrintf function to serial
//===========================================
// #define SerialMonitor

//===========================================
//WiFi connection
//===========================================
char ssid[] = ""; // WiFi Router ssid
char pass[] = ""; // WiFi Router password

//===========================================
//db server connection
//===========================================
const char* db_server = "";
const int db_port = 3309;
char* db_user = "";
char* db_pwd = "";
char* db_socket = "";


//===========================================
//Metric or Imperial measurements
//===========================================
//#define METRIC

//===========================================
//Anemometer Calibration
//===========================================
//I see 2 switch pulls to GND per revolation. Not sure what others see
#define WIND_TICKS_PER_REVOLUTION 2

//===========================================
//Set how often to wake and read sensors
//===========================================
//const int UpdateIntervalSeconds = 15 * 60;  //Sleep timer (900s) for my normal operation
const int UpdateIntervalSeconds = 5 * 60;  //Sleep timer (60s) testing

//===========================================
//Battery calibration
//===========================================
//measured battery voltage/ADC reading
#define batteryCalFactor .001167

#define MQTT
