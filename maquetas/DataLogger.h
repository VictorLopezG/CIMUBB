#include "SD.h"
#include "RTClib.h"
#include "SPI.h"
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

String extraerDatos(const char *archivo) {
  File registro = SD.open(("/"+String(archivo)), FILE_READ);
  String datos = "";
  char c;
  while (registro.available()) {
    c = registro.read();
    datos+= c;
  }
  registro.close();
  //Serial.println(datos);
  return datos;
}

int subirDatos( const char *archivo, String tMaq) {
  HTTPClient http;
  String hora, temp, hum, vis, flu;
  int httpcode;
  String datos = extraerDatos(archivo);
  datos = datos.substring(datos.indexOf('\n') + 1);
  do {
    String linea = datos.substring(0, datos.indexOf('\n') + 1);
    //Serial.println(linea);
    hora = linea.substring(0, linea.indexOf(' '));
    linea = linea.substring(linea.indexOf(' ') + 1);
    temp = linea.substring(0, linea.indexOf(' '));
    linea = linea.substring(linea.indexOf(' ') + 1);
    hum = linea.substring(0, linea.indexOf(' '));
    linea = linea.substring(linea.indexOf(' ') + 1);
    flu = linea.substring(0, linea.indexOf(' '));
    linea = linea.substring(linea.indexOf(' ') + 1);
    vis = linea.substring(0, linea.indexOf('\n'));
    String URL = "https://script.google.com/macros/s/AKfycbyk_oKwiIrhQlOdc6cdcX4MaF0KvVPJJG3S1Dwm10WpWYhMCqgII4DxzbM3_I8yCKaLGQ/exec?&tMaq="
     + tMaq +"&fecha="+String(archivo).substring(2) +"&hora=" + hora + "&temp=" + temp + "&hum=" + hum + "&flu=" + flu + "&vis=" + vis;
    Serial.println(URL);
    http.begin(URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    httpcode = http.GET();
    String payload=http.getString();
    if (httpcode != 200) {
      Serial.println(httpcode);
      break;
    }
    http.end();
    datos = datos.substring(datos.indexOf('\n') + 1);
  } while (!(datos.equals("")||datos.equals("\n")));
  return httpcode;
}

void revArch(const char *dirname, String tMaq) {
  File root = SD.open(dirname);
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
        Serial.println(String(file.name()).substring(2).c_str());
        if(200!=subirDatos(file.name(), tMaq)){
          Serial.println("Error de conexion");
          return;
        }
        SD.rename("/"+String(file.name()),("/"+String(file.name()).substring(2)).c_str());
      }
    }
    file = root.openNextFile();
  }
}

bool setReloj() {
  rtc.begin();
  if (!rtc.begin()) {
    Serial.println("No se encontro RTC");
    Serial.flush();
    return false;
  }
  if (rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  //-------------- TODO:
  } else {
    lastValidDate = rtc.now();
  }
  return true;
}

void writeFile( const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = SD.open(path, FILE_WRITE);
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

void appendFile( const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = SD.open(path, FILE_APPEND);
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
  if (!SD.exists("/" + reg + ".txt")||!SD.exists("/"+String(dia.day()) + "-" + String(dia.month()) + "-" + String(dia.year()))) {
    Serial.println("Registro de datos no encontrado");
    Serial.println("Creando registro de datos");
    writeFile(("/" + reg + ".txt").c_str(), "Hora Temp(°C) Humedad(%) Flujo(L/M) Visitas(N°) \n");
    return true;
  } else {
    Serial.println("Registro encontrado");
    return true;
  }
  return false;
}

void registrarDatos(float temp, float hum, float flu, int vis) {
  DateTime time = rtc.now();
  String datos = String(time.timestamp(DateTime::TIMESTAMP_TIME)) + " " + String(temp) + " " + String(hum) + " " + String(flu) + " " + String(vis) + "\n";
  appendFile(("/" + reg + ".txt").c_str(), datos.c_str());
}