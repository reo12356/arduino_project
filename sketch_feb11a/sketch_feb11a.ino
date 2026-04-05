#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_BMP3XX.h>
#include <MPU6050_light.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>



// SD 카드 CS핀
const int chipSelect = 7;

// 센서 객체
Adafruit_BMP3XX bmp;
MPU6050 mpu(Wire);

// 타이밍
unsigned long lastRead = 0;

void setup() {
  Serial.begin(9600);
  delay(500);
  
  Serial.println(F("=== Complete Sensor Logger ==="));
  
  // GPS
  gpsSerial.begin(9600);
  Serial.println(F("GPS OK"));
  
  // I2C 초기화
  Wire.begin();
  
  // BMP388 초기화
  if (bmp.begin_I2C(0x77) || bmp.begin_I2C(0x76)) {
    Serial.println(F("BMP OK"));
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_2X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  } else {
    Serial.println(F("BMP FAIL"));
  }
  
  // MPU6050 초기화
  if (mpu.begin() == 0) {
    Serial.println(F("MPU OK"));
    Serial.println(F("Calibrating... Keep flat!"));
    mpu.calcOffsets();  // 캘리브레이션 (5초간 평평하게!)
    Serial.println(F("Calibration done!"));
  } else {
    Serial.println(F("MPU FAIL"));
  }
  
  // SD 카드 초기화
  if (SD.begin(chipSelect)) {
    Serial.println(F("SD OK"));
    File f = SD.open("data.csv", FILE_WRITE);
    if (f) {
      // CSV 헤더
      f.println(F("Time,Lat,Lng,Alt,Temp,Press,AccX,AccY,AccZ,GyroX,GyroY,GyroZ,AngleX,AngleY"));
      f.close();
      Serial.println(F("CSV header written"));
    }
  } else {
    Serial.println(F("SD FAIL"));
  }
  
  Serial.println(F("System ready!\n"));
}

void loop() {
  // GPS 데이터 읽기 (항상 실행)
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }
  
  // MPU6050 업데이트 (각도 계산)
  mpu.update();
  
  // 100ms마다 센서 읽기 및 저장
  if (millis() - lastRead >= 100) {
    lastRead = millis();
    
    // MPU6050 각도 읽기
    float angleX = mpu.getAngleX();  // Roll (좌우 기울기)
    float angleY = mpu.getAngleY();  // Pitch (앞뒤 기울기)
    
    // 시리얼 출력 (모니터링용)
    Serial.print(F("Time: "));
    Serial.print(millis());
    Serial.print(F(" | AngleX: "));
    Serial.print(angleX, 1);
    Serial.print(F("° AngleY: "));
    Serial.print(angleY, 1);
    Serial.print(F("°"));
    
    Serial.print(F(" | GPS: "));
    if (gps.location.isValid()) {
      Serial.print(gps.location.lat(), 6);
      Serial.print(F(","));
      Serial.print(gps.location.lng(), 6);
    } else {
      Serial.print(F("N/A"));
    }
    
    Serial.print(F(" | BMP: "));
    if (bmp.performReading()) {
      Serial.print(bmp.temperature, 1);
      Serial.print(F("C "));
      Serial.print(bmp.pressure / 100.0, 1);
      Serial.print(F("hPa"));
    } else {
      Serial.print(F("N/A"));
    }
    
    Serial.print(F(" | MPU: "));
    Serial.print(mpu.getAccX(), 2);
    Serial.print(F(","));
    Serial.print(mpu.getAccY(), 2);
    Serial.print(F(","));
    Serial.println(mpu.getAccZ(), 2);
    
    // SD 카드에 저장
    File f = SD.open("data.csv", FILE_WRITE);
    if (f) {
      // 시간
      f.print(millis());
      f.print(F(","));
      
      // GPS 데이터
      if (gps.location.isValid()) {
        f.print(gps.location.lat(), 6);
        f.print(F(","));
        f.print(gps.location.lng(), 6);
        f.print(F(","));
        f.print(gps.altitude.meters(), 1);
      } else {
        f.print(F(",,,"));
      }
      f.print(F(","));
      
      // BMP388 데이터
      if (bmp.performReading()) {
        f.print(bmp.temperature, 1);
        f.print(F(","));
        f.print(bmp.pressure / 100.0, 1);
      } else {
        f.print(F(",,"));
      }
      f.print(F(","));
      
      // MPU6050 가속도
      f.print(mpu.getAccX(), 2);
      f.print(F(","));
      f.print(mpu.getAccY(), 2);
      f.print(F(","));
      f.print(mpu.getAccZ(), 2);
      f.print(F(","));
      
      // MPU6050 자이로
      f.print(mpu.getGyroX(), 2);
      f.print(F(","));
      f.print(mpu.getGyroY(), 2);
      f.print(F(","));
      f.print(mpu.getGyroZ(), 2);
      f.print(F(","));
      
      // MPU6050 각도
      f.print(angleX, 2);
      f.print(F(","));
      f.println(angleY, 2);
      
      f.close();
      Serial.println(F(">> Saved to SD"));
    } else {
      Serial.println(F(">> SD write failed"));
    }
  }
  
  delay(10);
}