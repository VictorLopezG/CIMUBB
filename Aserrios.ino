//bibliotecas para dispositivos e i2c
#include "Adafruit_VL53L0X.h"
#include <Wire.h>
#include <math.h>
#include "RTClib.h"
#include "SD.h"
#include <Adafruit_MLX90614.h>
#include <Adafruit_ADS1X15.h>
#include <SoftwareSerial.h>
//comunicacion serial para sensor de distancia
SoftwareSerial mySerial(16, 17);  //Define software serial, 16 is TX,17  is RX
char buff[4] = { 0x80, 0x06, 0x03, 0x77 };
unsigned char data[11] = { 0 };
//Reloj
RTC_DS1307 rtc;
DateTime tiempo;
DateTime lastValidDate;
//Se definen las direcciones i2c para todos los dispositivos
#define MLX_ADDRESS 0x5A
//modulo ads
#define ADS1_ADDRESS 0x48
// Direccion asignada para el sensor de distancia
//#define LOX1_ADDRESS 0x30  //
// Setea los pines XSHUT utilizados cuando se usan mas de 1 sensor de distancia
//#define SHT_LOX1 17  //14 en ESP32
// inicialización de la biblioteca
Adafruit_VL53L0X lox1 = Adafruit_VL53L0X();
//inicialización de la biblioteca modulo ads y sensor temp
Adafruit_ADS1115 ads;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
// mantiene la medición
float measure1;
//variables corriente
float Irms1;
float Irms2;
float Irms3;
float TA;
float voltaje = 380;
float raiz = 1.732;
float factordep = 0.84;
float Temp;
float Pot;
float corrientetotal = 0.00;
//Variables sensor distancia
float lectura = 0.0;
float lectura2 = 0.0;
float ajuste = 0.0;
float ajuste2 = 0.0;
float ancho = 0.0;
float distancia1 = 0.0;
float distancia2 = 0.0;
float anchoencm = 0.0;
int distanciafija = 212;
//Variables SD
long currentMillis = 0;
long previousMillis = 0;
unsigned long timerLog = 0, timerVis = 0;
byte pulse1Sec = 0;
//Constantes SD
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount;
//archivos
File myFile;

float distancia() {
  if (mySerial.available() > 0)  //Determine whether there is data to read on the serial
  {
    delay(50);
    for (int i = 0; i < 11; i++) {
      data[i] = mySerial.read();
    }
    unsigned char Check = 0;
    for (int i = 0; i < 10; i++) {
      Check = Check + data[i];
    }
    Check = ~Check + 1;
    if (data[10] == Check) {
      if (data[3] == 'E' && data[4] == 'R' && data[5] == 'R') {
        //Serial.println("Out of range");
        return -1;
      } else {
        float distance = 0;
        distance = (data[3] - 0x30) * 100 + (data[4] - 0x30) * 10 + (data[5] - 0x30) * 1 + (data[7] - 0x30) * 0.1 + (data[8] - 0x30) * 0.01 + (data[9] - 0x30);
        //Serial.print("Distance = ");
        //Serial.print(distance,3);
        //Serial.println(" M");
        delay(20);
        return distance;
      }
    }
  }
}
//Funciones para escribir en SD
void WriteFile(const char* path, const char* message) {
  myFile = SD.open(path, FILE_WRITE);
  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close();
    Serial.println("completed.");
  } else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}
//Agregar datos a un archivo de la SD
void appendFile(const char* path, String message) {
  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (!file.print(message)) {
    Serial.println("Append failed");
  }
  file.close();
}  // Registra los valores de las variables a medir en variables temporales para el registro en memoria
void registrarEnMemoria() {
  String fyh = String(tiempo.timestamp(DateTime::TIMESTAMP_DATE) + " " + tiempo.timestamp(DateTime::TIMESTAMP_TIME));
  String anchodecorte = String(ancho);
  String TemperaturaObjeto = String(Temp);
  String CorrienteIrms1 = String(Irms1);
  String CorrienteIrms2 = String(Irms2);
  String CorrienteIrms3 = String(Irms3);
  String distancia = String(ajuste);
  String PotenciaElectrica = String(Pot);
  appendFile("/registro.txt", fyh);
  appendFile("/registro.txt", " Distancia de sensor: " + distancia + " mm ; ");
  appendFile("/registro.txt", " Espesor de corte: " + anchodecorte + " mm ; ");
  appendFile("/registro.txt", " Temperatura objeto : " + TemperaturaObjeto + " (°C) ; ");
  appendFile("/registro.txt", " Corriente Irms1 : " + CorrienteIrms1 + " (A) ; ");
  appendFile("/registro.txt", " Corriente Irms2 : " + CorrienteIrms2 + " (A) ; ");
  appendFile("/registro.txt", " Corriente Irms3 : " + CorrienteIrms3 + " (A) ; ");
  appendFile("/registro.txt", " Potencia Electrica : " + PotenciaElectrica + "W");
  appendFile("/registro.txt", "\n");
  Serial.println("Datos registrados");
}
void setup() {  //Inicia las comunicaciones seriales e i2c de los sensores y dispositivos
  //!lox1.begin(LOX1_ADDRESS);
  Serial.begin(115200);
  Wire.begin();
  mlx.begin(MLX_ADDRESS);
  ads.begin(ADS1_ADDRESS);
  // espera hasta que el puerto usb se abra para dispositivos
  //while (!Serial) { delay(1); }
  //pinMode(SHT_LOX1, OUTPUT);
  //Serial.println(F("Shutdown pins inited..."));
  //digitalWrite(SHT_LOX1, LOW);
  //Serial.println(F("Both in reset mode...(pins are low)"));
  //Serial.println(F("Starting..."));
  rtc.begin();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  //-------------- TODO:
  } else {
    lastValidDate = rtc.now();
  }
  //inicializacion de SD
  Serial.println("Initializing SD card...");
  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    //return;
  } else {
    Serial.println("initialization done.");
  }
  mySerial.begin(9600);
  mySerial.print(buff);
}
void loop() {  //Obtención de la variable de distancia , en cual si la lectura es exitosa, se le corrigue un offset para lecturas menores a 40cm
  //lox1.rangingTest(&measure1, false);  // pass in 'true' to get debug data printout!
  // print sensor one reading
  if (distancia() != -1) {
    measure1 = distancia();
    ajuste = measure1 - 4;
    Serial.print(F("Distancia (mm): "));
    Serial.print(ajuste);
  } else {
    Serial.println(F("Fuera de rango"));
  }
  Serial.print(F("1: "));
  /*if (measure1.RangeStatus != 4) {  // if not out of range
    lectura = measure1.RangeMilliMeter;
    if (lectura < 400) {
      
      Serial.print(F("Distancia (mm): "));
      Serial.print(ajuste);
    } else {
      Serial.println(F("Out of range"));
    }
  }*/
  //calculo del espesor/ancho utilizando la resta entre refencia de la distancia del sensor hacia la cara de la sierra huincha y la medición obtenida del sensor
  ancho = (distanciafija - ajuste);
  Serial.print(F(" ; Espesor de corte: "));
  Serial.print(ancho);
  Serial.print(F(" mm ;"));
  Serial.println();
  Serial.print(F(" "));
  Temp = mlx.readObjectTempC();  //Temperatura obtenida del sensor de temperatura
  Serial.print("Temperatura Objeto: ");
  Serial.print(Temp);
  Serial.println("C");
  Irms1 = getCorriente1();  // Corriente RMS Fase 1 (R)
  Serial.print("Corriente SCT0131: ");
  Serial.print(Irms1);
  Serial.println(" A");
  Irms2 = getCorriente2();  // Corriente RMS Fase 2 (S)
  Serial.print("Corriente SCT0132: ");
  Serial.print(Irms2);
  Serial.println(" A");
  Irms3 = getCorriente3();  // Corriente RMS Fase 3 (T)
  Serial.print("Corriente SCT0133: ");
  Serial.print(Irms3);
  Serial.println(" A");
  corrientetotal = Irms1 + Irms2 + Irms3;
  Pot = voltaje * corrientetotal * factordep * raiz;  // Calculo de la potencia electrica
  Serial.print("Potencia Entrada: ");
  Serial.print(Pot);
  Serial.println(" W");
  Serial.println();  // Li­nea en blanco para mejor legibilidad
  //registro
  tiempo = rtc.now();
  unsigned long conteo = millis();
  //registro de informacion en SD
  if (conteo - timerLog >= 500) {  //cambio de 900000 a 10000
    timerLog = millis();
    registrarEnMemoria();
  }
}
float getCorriente1()  //Funciones para el calculo de corriente 1, 2 y 3. Escalamiento de señales de corriente a voltaje utilizando el módulo ADS
{
  float voltaje1;
  float corriente1;
  float sum1 = 0;
  long tiempo1 = millis();
  int counter1 = 0;
  while (millis() - tiempo1 < 1000) {
    voltaje1 = ads.readADC_SingleEnded(0) * 0.0625;
    corriente1 = voltaje1 * 100;
    corriente1 /= 120;
    sum1 += sq(corriente1);
    counter1++;
  }
  corriente1 = sqrt(sum1 / counter1);
  return corriente1;
}
float getCorriente2() {
  float voltaje2;
  float corriente2;
  float sum2 = 0;
  long tiempo2 = millis();
  int counter2 = 0;
  while (millis() - tiempo2 < 1000) {
    voltaje2 = ads.readADC_SingleEnded(1) * 0.0625;
    corriente2 = voltaje2 * 100;
    corriente2 /= 120;
    sum2 += sq(corriente2);
    counter2++;
  }
  corriente2 = sqrt(sum2 / counter2);
  return corriente2;
}
float getCorriente3() {
  float voltaje3;
  float corriente3;
  float sum3 = 0;
  long tiempo3 = millis();
  int counter3 = 0;
  while (millis() - tiempo3 < 1000) {
    voltaje3 = ads.readADC_SingleEnded(2) * 0.0625;
    corriente3 = voltaje3 * 100;
    corriente3 /= 120;
    sum3 += sq(corriente3);
    counter3++;
  }
  corriente3 = sqrt(sum3 / counter3);
  return corriente3;
}