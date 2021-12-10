const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -7 * 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;

//=======================================================================
//  printLocalTime: prints local timezone based time
//=======================================================================
void printLocalTime()
{
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.printf("Date:%02i %02i %i Time: %02i:%02i:%02i\n", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

//=======================================================================
//  printTimeNextWake: diagnostic routine to print next wake time
//=======================================================================
void printTimeNextWake( void)
{
  getLocalTime(&timeinfo);
  Serial.printf("Time to next wake: %i seconds\n", nextUpdate - mktime(&timeinfo) );
}

//=======================================================================
//  updateWake: calculate next time to wake
//=======================================================================
void updateWake (void)
{
  int muliplierBatterySave = 1;
  if (lowBattery)
  {
    muliplierBatterySave = 4;
  }
  getLocalTime(&timeinfo);
  //180 added to wipe out any RTC timing error vs NTP server - causing 2 WAKES back to back
  nextUpdate = mktime(&timeinfo) + UpdateIntervalSeconds * muliplierBatterySave + 180;
  nextUpdate = nextUpdate - nextUpdate % (UpdateIntervalSeconds * muliplierBatterySave);
  // Intentional offset for data aquire before display unit updates
  // guarantees fresh data
  if (nextUpdate > 120)
  {
    nextUpdate -= 60;
  }
}