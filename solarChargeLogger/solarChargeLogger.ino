
#include <SD.h>
#include <SPI.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define BENCHMARK true  // calculate how long each log takes
#define BENCHMARK_FUNCTIONS false  // calculate data for how long individual functions take, may increase loop time

#define SD_CS_PIN 10           // Chip select pin for the SD card module
#define LOG_INTERVAL_MS 1000  // Logging interval in milliseconds (15 seconds in this example)
#define ADC_READS 34          // Number of ADC reads to average
#define WRITE_LOG_TO_SERIAL true  // write the logged data to the serial port 
#define USE_SD_CARD true        // log the data to the sd card

// measurement Resistore
#define CURRENT_SHUNT_OHMS  5.0  // value of resistor being used to sense current
#define VOLTAGE_DIVIDER_R1_OHMS 15000
#define VOLTAGE_DIVIDER_R2_OHMS 1000

#define COMPENSATE_CURRENT_SHUNT_VOLTAGE false  // compensate for the fact the current shunt voltage pushes the ground for the battery voltage measurement down below battery ground.

// Variables for battery measurement
const int batteryPin = A0;
const int currentPin = A1;

String logFileName;

// Variables for timekeeping
unsigned long previousMillis = 0;

// Variables to store measurements to average
uint16_t batteryRaw[ADC_READS], currentRaw[ADC_READS];

void setup() {
  Serial.begin(9600);
  analogReference(INTERNAL);  // set 1.1V internal reference

  if (USE_SD_CARD){ 
    // Initialize SD card
   if (!SD.begin(SD_CS_PIN)) {
     Serial.println("SD card initialization failed!");
     return;
   }
    // Create a new file for logging
    logFileName = getLogFileName();

    File dataFile = SD.open(logFileName, FILE_WRITE);
    if (dataFile) {
      dataFile.println("Time,Voltage(mV),current(mA)");
      dataFile.close();
    } else {
      Serial.println("Error creating log file!");
    }
  }
}

void loop() {
 
  if(millis() - previousMillis > LOG_INTERVAL_MS) // if time elapsed
  {
    // Update the previousMillis variable
    previousMillis = millis();
  
    // Calculate the battery voltage
    for(int n=0;n<ADC_READS;n++){ 
      currentRaw[n] = analogRead(currentPin);
      batteryRaw[n] = analogRead(batteryPin);  
    }
    if(COMPENSATE_CURRENT_SHUNT_VOLTAGE){
      for(int n=0;n<ADC_READS;n++){
        batteryRaw[n] = rawToBatteryVoltagemv(batteryRaw[n]) - rawToCurrentmv(currentRaw[n]);
      }
      logData(previousMillis, calculateAverage(batteryRaw, ADC_READS), rawToCurrentMa(calculateAverage(currentRaw, ADC_READS)) );
    }else{
    // Log the data to the SD card
    logData(previousMillis, rawToBatteryVoltagemv(calculateAverage(batteryRaw, ADC_READS)), rawToCurrentMa(calculateAverage(currentRaw, ADC_READS)) );
    }
    
    if(BENCHMARK){
      Serial.println("looptime(ms): " + String( millis() - previousMillis));
    }
  }

}

uint16_t calculateAverage(uint16_t data[], int count){
  int highestValueIndex = 0, lowestValueIndex = 0, n;
  uint32_t total = 0;
  unsigned long startMillis;
  if(BENCHMARK_FUNCTIONS){
    startMillis = millis();
  }

  if(count<=0)
    return 0; // invalid input data

  if(count<4) {
    // if not enough values to remove highest and lowest and average
    for(n=0;n<count;n++)
      total = total + data[n];
    return (uint16_t)(total/count);
  }

  for(n=1;n<count;n++){
    if(data[n] > data[highestValueIndex])
      highestValueIndex = n;
    if(data[n] < data[lowestValueIndex])
      lowestValueIndex = n;
  }
  int countedValues = 0;
  for(n=0;n<count;n++){
    if(n!= lowestValueIndex && n!= highestValueIndex){
      total+= data[n];
      countedValues++;  // Use this for case of highest and lowest being both the same to avoid bug from count-2
    }
  }
  if(BENCHMARK_FUNCTIONS){
    Serial.println("calcAvgtime(ms): " + String( millis() - startMillis));

  }

  return (uint16_t)(total/countedValues);

}


uint16_t rawToBatteryVoltagemv(uint16_t raw) {
  //                          mv/ADCunit        (R1 + R2) / R2
  float voltage = raw * (1100.0 / 1023.0) * (float)(VOLTAGE_DIVIDER_R1_OHMS + VOLTAGE_DIVIDER_R2_OHMS)/(float)VOLTAGE_DIVIDER_R2_OHMS;
  return (uint16_t) voltage;
}

uint16_t rawToCurrentMa(uint16_t raw){
  uint16_t result;
  result = (uint16_t)((float)raw *(1100.0 / 1023.0) / CURRENT_SHUNT_OHMS);
  return result;
}

uint16_t rawToCurrentmv(uint16_t raw){
  uint16_t result;
  result = (uint16_t)((float)raw *(1100.0 / 1023.0));
  return result;
}


String millisToHMS(unsigned long millis){
  unsigned long seconds = millis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  seconds %= 60;
  minutes %= 60;
 
  return String( String(hours) + ":" + String(minutes) + ":" + String(seconds) );
}

void logData(unsigned long timestamp, uint16_t batteryVoltage, uint16_t current) {
  unsigned long startMillis;
  if(BENCHMARK_FUNCTIONS){
    startMillis = millis();
  }

  String timestampString = millisToHMS(timestamp);
  String logLine = String(timestampString + "," + String(batteryVoltage) + "," + String(current) + "\n");

  if(WRITE_LOG_TO_SERIAL)
    Serial.print(logLine);

  if(BENCHMARK_FUNCTIONS){
      Serial.println("logstringtime(ms): " + String( millis() - startMillis));
      startMillis = millis();
  }

  if(USE_SD_CARD){ 
    File dataFile = SD.open(logFileName, FILE_WRITE | O_CREAT | O_APPEND);
    if (dataFile) {
      dataFile.print(logLine);
      Serial.println("Data logged.");
      dataFile.close();
    } else {
      Serial.println("Error opening log file!");
    }
    
    if(BENCHMARK_FUNCTIONS){
      Serial.println("logSDtime(ms): " + String( millis() - startMillis));
    }
  }
}

String getLogFileName() {
  String baseFileName = "log";
  String fileExt = ".csv";
  String fileName = baseFileName + "01" + fileExt;
  int fileCount = 1;

  while (SD.exists(fileName)) {
    // Generate a new file name with a numbered suffix
    if(fileCount < 10){
    fileName = baseFileName +  "0" + String(fileCount) + fileExt;
    }else{
    fileName = baseFileName + String(fileCount) + fileExt;    
    }
    fileCount++;
  }

  return fileName;
}
