#include <WiFi.h>
#include <DHT.h>
#include "RTClib.h"
#include "ESPAsyncWebServer.h"
#include "SD.h"
#include "SPI.h"
#include <ESP32Servo.h>
#include "WebSocketsServer.h"
#include "HTTPClient.h"
//motores
Servo servo1;
Servo servo2;
int minUs = 500;
int maxUs = 2500;
int servopin1 = 14;
int servopin2 = 12;
int VENT_PIN = 26;  //ventilador
//temporizador
unsigned long timer;
//luz
#define LUZ_PIN 4
//sensor T° y humedad
DHT dht(17, DHT11);
DHT dht1();  //segundo sensor T°
//id del script para subir datos a google sheets
String id_Script = "AKfycbwUC_8WaJJ-YYV459QiljC8qZYUkk2WyUsOUDCefGgl7rtkeZ2fuve0ULvumT5q02Y5Ng";
//pagina web
AsyncWebServer server(80);
//pagina web en tiempo real
WebSocketsServer webSocket = WebSocketsServer(81);
//archivos
File myFile;
//reloj
RTC_DS1307 rtc;
DateTime lastValidDate;
//registro
float tempin, tempex, hum;
int luz, td = 20, modo = 1;
String estado, aire;
//Funciones
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
}
//registra los datos
void registrarEnMemoria() {
  String fecha = String(lastValidDate.timestamp(DateTime::TIMESTAMP_DATE));
  String hora = String(lastValidDate.timestamp(DateTime::TIMESTAMP_TIME));
  appendFile("/Datos.txt", fecha);
  appendFile("/Datos.txt", hora);
  appendFile("/Datos.txt", " " + String(tempin) + " °C ");
  appendFile("/Datos.txt", " " + String(tempex) + " °C ");
  appendFile("/Datos.txt", " " + String(td) + " °C ");
  appendFile("/Datos.txt", String(hum) + " % ");
  appendFile("/Datos.txt", String(luz) + " ");
  appendFile("/Datos.txt", estado + " ");
  appendFile("/Datos.txt", "\n");
  Serial.println("datos registrados");
  String URL_Final = "https://script.google.com/macros/s/" + id_Script + "/exec?&date="
                     + fecha + "&hora=" + hora + "&tempin=" + tempin + "&tempex=" + tempex
                     + "&td=" + td + "&hum=" + hum + "&luz=" + luz + "&est=" + estado + "&aire=" + aire;
  HTTPClient http;
  //Serial.println(URL_Final);
  http.begin(URL_Final.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpcode = http.GET();
  String payload;
  if (httpcode > 0) {
    //payload = http.getString();
    //Serial.println(payload);
    Serial.println(httpcode);
  }
  http.end();
}

void mediciones() {
  tempin = dht.readTemperature();
  tempex = dht.readTemperature();
  hum = dht.readHumidity();
}

int estacion() {
  if ((lastValidDate.month() > 3 && lastValidDate.month() < 9)) {  //horario para invierno
    return 0;
  }
  if ((lastValidDate.month() < 3 || lastValidDate.month() > 9)) {  //horario verano
    return 1;
  }
  if (lastValidDate.month() == 3 && lastValidDate.day() >= 20) {  //horario para invierno
    return 0;
  } else {
    if (lastValidDate.month() == 3) {  //horario verano
      return 1;
    }
  }
  if (lastValidDate.month() == 9 && lastValidDate.day() >= 23) {  //horario verano
    return 1;
  } else {
    if (lastValidDate.month() == 9) {  //horario para invierno
      return 0;
    }
  }
}

void cInvi() {
  if (estado.equalsIgnoreCase("Cerrado_Invierno")) {
    return;
  }
  servo1.attach(servopin1, minUs, maxUs);
  servo1.write(0);
  if (estado.equalsIgnoreCase("Cerrado_Verano")) { 
    delay(1000);  
  }
  if (estado.equalsIgnoreCase("Abierto")) {
    delay(500);  
  }
  estado = "Cerrado_Invierno";
  servo1.detach();
}

void cVera() {
  if (estado.equalsIgnoreCase("Cerrado_Verano")) {
    return;
  }
  servo1.attach(servopin1, minUs, maxUs);
  servo1.write(180);
  if (estado.equalsIgnoreCase("Abierto")) {
    delay(500);
  }
  if (estado.equalsIgnoreCase("Cerrado_Invierno")) {
    delay(1000);
  }
  estado = "Cerrado_Verano";
  servo1.detach();
}

void abrirPersiana() {
  if (estado.equalsIgnoreCase("Abierto")) {
    return;
  }
  servo1.attach(servopin1, minUs, maxUs);
  if (estado.equalsIgnoreCase("Cerrado_Invierno")) {
    servo1.write(180);
  }
  if (estado.equalsIgnoreCase("Cerrado_Invierno")) {
    servo1.write(0);
  }
  delay(500);
  servo1.detach();
  estado = "Abierto";
}

void abrirAire() {
  if (aire.equalsIgnoreCase("abierto")) {
    return;
  }
  servo2.attach(servopin2, minUs, maxUs);
  servo2.write(180);
  delay(500);
  aire = "Abierto";
  servo1.detach();
}
void cerrarAire() {
  if (aire.equalsIgnoreCase("cerrado")) {
    return;
  }
  servo2.attach(servopin2, minUs, maxUs);
  servo2.write(0);
  delay(500);
  aire = "Cerrado";
  servo1.detach();
}

void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  Serial.print("reciviendo evento");
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("[%u] Cliente conectado.\n", num);
      // Habilitar el modo binario para mejorar el rendimiento
      webSocket.sendBIN(num, (uint8_t*)"1", 1);  // Envía un pequeño mensaje binario al cliente
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Cliente desconectado.\n", num);
      break;
    case WStype_TEXT:
      Serial.printf("[%u] Mensaje recibido: %s\n", num, payload);
      String msg = String((char*)(payload));
      if (msg.toInt() >= 10 && msg.toInt() <= 30) {
        td = msg.toInt();
      }
      if (msg.equalsIgnoreCase("manual")) {
        modo = 0;
      }
      if (msg.equalsIgnoreCase("auto")) {
        modo = 1;
      }
      if (modo == 0) {
        if (msg.equalsIgnoreCase("abrir")) {
          abrirPersiana();
        }
        if (msg.equalsIgnoreCase("cerrarI")) {
          cInvi();
        }
        if (msg.equalsIgnoreCase("cerrarV")) {
          cVera();
        }
        if (msg.equalsIgnoreCase("on")) {
          abrirAire();
          digitalWrite(VENT_PIN, HIGH);
        }
        if (msg.equalsIgnoreCase("off")) {
          cerrarAire();
          digitalWrite(VENT_PIN, LOW);
        }
      }
      break;
  }
}

void enviarDatosWebSocket(int luz, String estado, float tempin, float tempex, float hum, String aire, int modo, int td) {
  // Crear una cadena con los datos
  String mensaje = luz + "," + estado + "," + tempin + "," + tempex + "," + hum + "," + aire + "," + modo + "," + td;
  // Enviar la cadena a todos los clientes conectados
  webSocket.broadcastTXT(mensaje);
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

void setup() {
  Serial.begin(115200);
  //inicializar la tarjeta SD y buscar registros o crearlos
  SD.begin(5);
  if (!SD.exists("/Datos.txt")) {
    Serial.println("Registro de datos no encontrado");
    Serial.println("Creando registro de datos");
    myFile = SD.open("/Datos.txt", FILE_WRITE);
    myFile.println("Fecha hora Temp_Int Temp_Ext Temp_Des Humedad Luz Estado");
    myFile.close();
  } else {
    Serial.println("Registro encontrado");
  }
  //buscar credenciales del wifi y conectarse;
  String SSID = "";
  String CLAVE = "";
  if (!SD.exists("/Credenciales.txt")) {
    Serial.println("Registro de credenciales no encontrado");
    Serial.println("Creando registro de datos");
    myFile = SD.open("/Credenciales.txt", FILE_WRITE);
    myFile.print("Red:\nClave:");
    myFile.close();
    Serial.println("Por favor registre el nombre de la red y clave en el archivo de la tarjeta");
  } else {
    Serial.println("Credenciales encontradas");
    myFile = SD.open("/Credenciales.txt", FILE_READ);
    SSID = extraerCredenciales();
    Serial.println(SSID);
    CLAVE = extraerCredenciales();
    Serial.println(CLAVE);
    myFile.close();
    //wifi
    WiFi.begin(SSID.c_str(), CLAVE.c_str());
    Serial.println("Conectandose a la red:");
    Serial.println(SSID);
    timer = millis();
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
      if ((millis() - timer) > 20000) {
        Serial.println("\n no se pudo conectar a la red");
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("Conectado a la red " + SSID);
      Serial.println("IP:");
      Serial.println(WiFi.localIP());
    }
  }
  //iniciando sensores de T°
  dht.begin();
  //iniciando reloj
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
  //ajustar motores a una posicion
  abrirPersiana();
  cerrarAire();
  pinMode(VENT_PIN, OUTPUT);
  //monitoreo web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SD, "/Persiana.html", String(), false);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(handleWebSocketEvent);
  //registro inicial de datos
  mediciones();
  registrarEnMemoria();
}

void loop() {
  mediciones();
  webSocket.loop();
  enviarDatosWebSocket(luz, estado, tempin, tempex, hum, aire, modo, td);
  //manejo automatico de la persiana y camara de aire
  if (modo == 1) {
    if (estacion() == 0) {  //invierno
      if (tempin <= td && aire.equalsIgnoreCase("abierto")) {
        cerrarAire();
      }
      if (tempin > td && aire.equalsIgnoreCase("cerrado")) {
        abrirAire();
      }
    }
    if (estacion() == 1) {
      if (tempex >= td && estado.equalsIgnoreCase("Abierto")) {
        cVera();
      }
      if (tempex < td && (estado.equalsIgnoreCase("cerrado_invierno") || estado.equalsIgnoreCase("cerrado_verano"))) {
        abrirPersiana();
      }
    }
  }
  if ((millis() - timer) > 900000) {
    timer = millis();
    registrarEnMemoria();
  }
}
