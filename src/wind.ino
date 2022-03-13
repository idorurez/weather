//=======================================================
// Variables used in calculating the windspeed (from ISR)
//=======================================================
volatile unsigned long lastWindCheck = 0;
volatile unsigned long lastTick = 0;
volatile int windClicks = 0;

//========================================================================
//  readWindSpeed: Look at ISR data to see if we have wind data to average
//========================================================================
void readWindSpeed(struct sensorData *environment )
{
  float deltaTime = millis() - lastWindCheck;

  deltaTime /= 1000.0; // convert to seconds

  float windSpeed = (float) windClicks / deltaTime;

  windClicks = 0;
  lastWindCheck = millis(); 
  windSpeed *= 1.492;
  
  Serial.println("WindSpeed: " + String(windSpeed));
  environment->windSpeed = windSpeed;
}

//=======================================================
//  readWindDirection: Read ADC to find wind direction
//=======================================================
//This function is in testing mode now
void readWindDirection(struct sensorData *environment)
{
  int windPosition;
  //Initial direction
  //Prove it is not this direction
  String windDirection = "0";
  String windCardinalDirection = "N";
  int analogCompare[15] = {150, 300, 450, 600, 830, 1100, 1500, 1700, 2250, 2350, 2700, 3000, 3200, 3400, 3900};
  String windDirText[15] = {"157.5", "180", "247.5", "202.5", "225", "270", "292.5", "112.5", "135", "337.5", "315", "67.5", "90", "22.5", "45"};
  String windDirCardinalText[15] = {"SSE", "S", "WSW", "SSW", "SW", "W", "WNW", "ESE", "SE", "NNW", "NW", "ENE", "E", "NNE", "NE"};
  char buffer[10];
  int vin = analogRead(WIND_DIR_PIN);

  for (windPosition = 0; windPosition < 15; windPosition++)
  {
    if (vin < analogCompare[windPosition])
    {
      windDirection = windDirText[windPosition];
      windCardinalDirection = windDirCardinalText[windPosition];
      break;
    }
  }
  Serial.printf("Analog value: %i Wind direction: %s  \n", vin, windDirection);
  windDirection.toCharArray(buffer, 5);
  environment->windDir = atof(buffer);
  strcpy(environment->windCardDir, windCardinalDirection.c_str());
}

//=======================================================
//  windTick: ISR to capture wind speed relay closure
//=======================================================
void IRAM_ATTR windTick(void)
{
  long timeSinceLastTick = millis() - lastTick;
  if (timeSinceLastTick > 10)
  {
    lastTick = millis();
    windClicks++;
  }
}
