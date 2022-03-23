//=======================================================
//  readSensors: Read all sensors and battery voltage
//=======================================================
//Entry point for all sensor data reading
void readSensors(struct sensorData *environment)
{
  readWindSpeed(environment);
  readWindDirection(environment);
  readTemperature(environment);
  readLux(environment);
  readBME(environment);
  readUV(environment);
  readBattery(environment);
}

//=======================================================
//  readTemperature: Read 1W DS1820B
//=======================================================
void readTemperature (struct sensorData *environment)
{
  Serial.println("Requesting temperatures...\n");
  
  temperatureSensor.requestTemperatures();
  Serial.println("DONE");
  environment->tempC = temperatureSensor.getTempCByIndex(0);

  // Check if reading was successful
  if (environment->tempC != DEVICE_DISCONNECTED_C) {
    environment->tempF = environment->tempC * 9 / 5 + 32;
    Serial.printf("Temperature for the device 1 (index 0) is: %5.1f C: %5.1f F\n", environment->tempC, environment->tempF);
  } else {
    Serial.println("Error: Could not read temperature data\n");
    environment->tempF = -40;
    environment->tempC = -40;
  }
}

//=======================================================
//  readBattery: read analog volatage divider value
//=======================================================
void readBattery (struct sensorData *environment)
{
  int val;
  float Vout;
  val = analogRead(VOLT_PIN);
  // Vout = Dout * Vmax / Dmax, Dmax is maximum output adc raw digital result, for esp32 it is 4095 with single or continuous read mode
  // Vout = val * (3.3 / 4095.0); // formula for calculating voltage out 
  Vout = val * 0.0008058608;
  // Battery voltage = Vout * ( R2+R1) / R2
  // Battery voltage = val * (3.3 / 4095) * 1.27
  environment->batteryAdc = Vout;
  environment->batteryVoltage = Vout * 1.27;
  Serial.printf("Battery digital :%i voltage: %6.2f\n", val, environment->batteryVoltage);
  //check for low battery situation
  if (environment->batteryVoltage < 3.78)
  {
    lowBattery = true;
  }
  else
  {
    lowBattery = false;
  }
}

//=======================================================
//  readLux: LUX sensor read
//=======================================================
void readLux(struct sensorData *environment)
{
  if (lightMeter.measurementReady()) {
    environment->lux = lightMeter.readLightLevel();
    Serial.println("LUX value: " +  String(environment->lux));
  } else {
    Serial.println("BH1750 not ready!");
  }
}

//=======================================================
//  readBME: BME sensor read
//=======================================================
void readBME(struct sensorData *environment) {
  Serial.println("Attempting to read bme680");
  String output;
  unsigned long time_trigger = millis();
  if (iaqSensor.run()) { // If new data is available
    output = String(time_trigger);
    output += ", " + String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.rawHumidity);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    output += ", " + String(iaqSensor.pressure);
    output += ", " + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.iaq);
    output += ", " + String(iaqSensor.iaqAccuracy);
    output += ", " + String(iaqSensor.staticIaq);
    output += ", " + String(iaqSensor.co2Equivalent);
    output += ", " + String(iaqSensor.breathVocEquivalent);
    Serial.println(output);

    environment->bsecRawTemp = iaqSensor.rawTemperature;
    environment->bsecRawHumidity = iaqSensor.rawHumidity;
    environment->bsecTemp = iaqSensor.temperature;
    environment->bsecHumidity = iaqSensor.humidity;
    environment->bsecPressure = iaqSensor.pressure;
    environment->bsecGasResistance = iaqSensor.gasResistance;
    environment->bsecIaq = iaqSensor.iaq; 
    environment->bsecIaqAccuracy = iaqSensor.iaqAccuracy; 
    environment->bsecStaticIaq = iaqSensor.staticIaq; 
    environment->bsecCo2Equiv = iaqSensor.co2Equivalent; 
    environment->bsecBreathVocEquiv = iaqSensor.breathVocEquivalent;
  }
  Serial.println("Done attempting to read BME680");
}

//=======================================================
//  readUV: get implied uv sensor value
//=======================================================
void readUV(struct sensorData *environment)
{ 
  environment->uvIndex = uv.readUV() / 100;
  environment->visLight = uv.readVisible();
  environment->infLight = uv.readIR();
  Serial.println("UV Index: " + String(environment->uvIndex));
  Serial.println("Vis: " + String(uv.readVisible()));
  Serial.println("IR: " + String(uv.readIR()));
}
