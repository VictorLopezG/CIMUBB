#include "SD.h"
#include "RTClib.h"
#include "SPI.h"
#include "FS.h"
#include "HTTPClient.h"

File myFile;
RTC_DS1307 rtc;
DateTime lastValidDate;
String reg;

bool setSD() {
  if (!SD.begin(5)) {
    Serial.println("Error al iniciar la tarjeta SD");
    return false;
  } else {
    Serial.println("Tarjeta SD iniciada correctamente");
    return true;
  }
}

void subirDatos(const char *archivo){

}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      if (String(file.name()).startsWith("P-")) {
        subirDatos(file.name());
        fs.rename(file.name(),String(file.name()).substring(3).c_str());
      }
    }
  }
  file = root.openNextFile();
}

bool setReloj() {
  rtc.begin();
  if (!rtc.begin()) {
    Serial.println("No se encontro RTC");
    Serial.flush();
    return false;
  }
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  //-------------- TODO:
  } else {
    lastValidDate = rtc.now();
  }
  return true;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

bool setRegistros() {
  DateTime dia = rtc.now();
  reg = "P-" + String(dia.day()) + "-" + String(dia.month()) + "-" + String(dia.year());
  if (!SD.exists("/" + reg + ".txt")) {
    Serial.println("Registro de datos no encontrado");
    Serial.println("Creando registro de datos");
    writeFile(SD, ("/" + reg + ".txt").c_str(), "Hora Temp Humedad Flujo Visitas");
    return true;
  } else {
    Serial.println("Registro encontrado");
    return true;
  }
  return false;
}

String extraerCredenciales() {
  String cred = "";
  char c;
  while (myFile.available()) {
    c = myFile.read();
    cred += c;
    if (cred.endsWith(":")) {
      cred = "";
      break;
    };
  }
  while (myFile.available()) {
    c = myFile.read();
    cred += c;
    if (myFile.peek() == '\n' || !myFile.available()) {
      break;
    }
  }
  return cred;
}

void registrarDatos(float temp, float hum, float flu, int vis) {
  DateTime time = rtc.now();
  String datos = String(time.timestamp(DateTime::TIMESTAMP_FULL)) + " " + String(temp) + "°C " + String(hum) + "% " + String(flu) + "L/M " + String(vis);
  appendFile(SD, ("/" + reg + ".txt").c_str(), datos.c_str());
}