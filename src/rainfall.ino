// Variables used in software delay to supress spurious counts on rain_tip
volatile unsigned long timeSinceLastTip = 0;
volatile unsigned long validTimeSinceLastTip = 0;
volatile unsigned long lastTip = 0;

//=======================================================================
//  clearRainfall: zero out rainfall counter structure
//=======================================================================
void clearRainfall(void)
{
  memset(&rainfall, 0x00, sizeof(rainfall));
}

//=======================================================================
//  clearRainfallHour: zero out specific hour element of rainfall structure array
//=======================================================================
void clearRainfallHour(int hourPtr)
{
  rainfall.hourlyRainfall[hourPtr % 24] = 0;
}

//=======================================================================
//  addTipsToHour: increment current hour tip count
//=======================================================================
void addTipsToHour(int count)
{
  int hourPtr = timeinfo.tm_hour;
  rainfall.hourlyRainfall[hourPtr] = rainfall.hourlyRainfall[hourPtr] + count;
}

//=======================================================================
//  printHourlyArray: diagnostic routine to print hourly rainfall array to terminal
//=======================================================================
void printHourlyArray (void)
{
  int hourCount = 0;
  for (hourCount = 0; hourCount < 24; hourCount++)
  {
    Serial.printf("Hour %i: %u\n", hourCount, rainfall.hourlyRainfall[hourCount]);
  }
}

//=======================================================================
//  last24: return tip counter for last 24h (technically 23h)
//=======================================================================
int last24(void)
{
  int hour;
  int totalRainfall = 0;
  for (hour = 0; hour < 24; hour++)
  {
    totalRainfall += rainfall.hourlyRainfall[hour];
  }
  Serial.printf("Total rainfall: %i\n", totalRainfall);
  return totalRainfall;
}

//=======================================================================
//  rainTick: ISR for rain tip gauge count
//=======================================================================
//ISR
void IRAM_ATTR rainTick(void)
{
  timeSinceLastTip = millis() - lastTip;
  //software debounce attempt
  if (timeSinceLastTip > 400)
  {
    validTimeSinceLastTip = timeSinceLastTip;
    rainTicks++;
    lastTip = millis();
  }
}
