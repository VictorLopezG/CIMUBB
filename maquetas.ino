#include "DHT.h"
#include "DataLogger.h"
#include "WiFi.h"
//#include "LiquidCrystal_I2C.h"

#define pin_PIR 12
#define pin_Flujo 13
#define pin_boton 14
#define pin_LED 25
#define pin_DHT 17
#define pin_bom 16
//LiquidCrystal_I2C lcd(0x27,16,2);

float temp = 0, flu, hum = 0;
long previousMillis = 0;
long currentMillis = 0;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
unsigned long lTimer = 0;
unsigned long timer = 0;
String tMaq = "";
DHT dht(pin_DHT, DHT11);
bool uPIR = false, blink = false, led = false;
unsigned int visitas = 0;

bool lecturaPIR() {
  int pir = digitalRead(pin_PIR);
  if (pir == HIGH) {
    return true;
  } else {
    return false;
  }
}

void IRAM_ATTR bomba() {
  detachInterrupt(pin_boton);
  unsigned long iTimer = millis();
  digitalWrite(pin_bom, LOW);
  if (millis() - iTimer >= 3000) {
    digitalWrite(pin_bom, HIGH);
    attachInterrupt(digitalPinToInterrupt(pin_bom), bomba, FALLING);
  }
}

void flujo() {
  float flowRate;
  currentMillis = millis();
  if (currentMillis - previousMillis > 1000) {

    pulse1Sec = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    flu = flowRate;
  }
}

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

void mediciones() {
  if (tMaq.equalsIgnoreCase("atrapanieblas")) {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
  }
  flujo();
}

String extraerCredenciales(File myFile) {
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

  //lcd.init();
  //lcd.backlight();
  //lcd.clear();

  Serial.println("iniciando");
  if (!setSD()) {
    Serial.println("SD no encontrada");
  }
  if (!setReloj()) {
    Serial.println("Reloj no encontrado");
  }
  if (!setRegistros()) {
    Serial.println("Error en los registros");
  }

  //buscar credenciales del wifi y conectarse;
  File myFile;
  String SSID = "";
  String CLAVE = "";
  if (!SD.exists("/Credenciales.txt")) {
    Serial.println("Registro de credenciales no encontrado");
    Serial.println("Creando registro de datos");
    myFile = SD.open("/Credenciales.txt", FILE_WRITE);
    myFile.print("Red:\nClave:\nTipo:");
    myFile.close();
    Serial.println("Por favor registre el nombre de la red, clave y tipo de maqueta en el archivo de la tarjeta");
  } else {
    Serial.println("Credenciales encontradas");
    myFile = SD.open("/Credenciales.txt", FILE_READ);
    SSID = extraerCredenciales(myFile);
    Serial.println(SSID);
    CLAVE = extraerCredenciales(myFile);
    Serial.println(CLAVE);
    tMaq = extraerCredenciales(myFile);
    Serial.println(tMaq);
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
  if (tMaq.equalsIgnoreCase("Atrapanieblas")) {
    dht.begin();
    Serial.println("iniciando DHT11");
  }
  revArch("/", tMaq);
  //lcd.begin(16, 2);
  pinMode(pin_bom, OUTPUT);  //bomba
  digitalWrite(pin_bom, HIGH);
  pinMode(pin_LED, OUTPUT);          //led
  pinMode(pin_PIR, INPUT);           //pir
  pinMode(pin_boton, INPUT_PULLUP);  //boton
  attachInterrupt(digitalPinToInterrupt(pin_bom), bomba, FALLING);
  pinMode(pin_Flujo, INPUT);  //flujo
  attachInterrupt(digitalPinToInterrupt(pin_Flujo), pulseCounter, FALLING);
  //esp_sleep_enable_timer_wakeup(1000000 * 3600 * 13);
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long timer1, timer2;
  mediciones();
  /*lcd.setCursor(0, 0);
  lcd.print("Flujo:" + String(flu));
  lcd.setCursor(0, 1);
  lcd.print("Temp:" + String(temp) + "Hum:" + String(hum));*/
  if (lecturaPIR() && !uPIR) {
    visitas++;
    uPIR = true;
    registrarDatos(temp, hum, flu, visitas);
    timer1 = millis();
    timer2 = millis();
    do {
      if (millis() > timer2 + 10000) {
        mediciones();
        registrarDatos(temp, hum, flu, visitas);
        timer2 = millis();
      }
      if (timer1 - millis() > 60000) {
        break;
      }
    } while (1);
    mediciones();
    registrarDatos(temp, hum, flu, visitas);
  }
  if (!lecturaPIR() && uPIR) {
    uPIR = false;
  }
  if (String(rtc.now().timestamp(DateTime::TIMESTAMP_TIME)).equals("23:59:59")) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Subiendo_datos");
      revArch("/", tMaq);
      blink = true;
    } else {
      blink = false;
    }
    visitas = 0;
    //esp_deep_sleep_start();
  }
  if (blink) {
    if (led && millis() - lTimer > 1000) {
      digitalWrite(pin_LED, HIGH);
    }
    if (!led && millis() - lTimer > 1000) {
      digitalWrite(pin_LED, LOW);
    }
  } else {
    digitalWrite(pin_LED, HIGH);
  }
}