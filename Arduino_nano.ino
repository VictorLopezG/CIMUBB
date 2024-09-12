#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

#define S1 A0
#define S2 A1
#define S3 A2
#define S4 A3

const int chipSelect = 10;
RTC_DS1307 rtc;
int a, aa, b, bb, c, cc, d, dd;
File reg;
void regDat() {
  //Serial.println("registrando");
  reg = SD.open("datos.txt", FILE_WRITE);
  if (reg) {
    reg.println(rtc.now().timestamp(DateTime::TIMESTAMP_DATE) + " " + rtc.now().timestamp(DateTime::TIMESTAMP_TIME) + " " + String(!a) + " " + String(!b) + " " + String(!c) + " " + String(!d));
    reg.close();
  } else {
    Serial.println("error al abrir el archivo");
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed.");
  }
  Serial.println("initialization done.");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!SD.exists("datos.txt")) {
    Serial.println("Creando registro");
    reg = SD.open("datos.txt", FILE_WRITE);
    reg.println("Fecha Hora S1 S2 S3 S4");
    reg.close();
  }
  //Los sensores detectan en bajo
  pinMode(S1, INPUT_PULLUP);
  pinMode(S2, INPUT_PULLUP);
  pinMode(S3, INPUT_PULLUP);
  pinMode(S4, INPUT_PULLUP);

  a = digitalRead(S1);
  aa = a;
  b = digitalRead(S2);
  bb = b;
  c = digitalRead(S3);
  cc = c;
  d = digitalRead(S4);
  dd = d;
}

void loop() {
  a = digitalRead(S1);
  b = digitalRead(S2);
  c = digitalRead(S3);
  d = digitalRead(S4);
  
  if (a != aa||b != bb||c != cc||d != dd) {
    aa = a;
    bb = b;
    cc = c;
    dd = d;
    //regDat();
    Serial.print(!a);
    Serial.print(!b);
    Serial.print(!c); 
    Serial.println(!d);
  }
}