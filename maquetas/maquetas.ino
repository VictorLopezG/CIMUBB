#include "DHT.h"
#include "DataLogger.h"
#include "WiFi.h"
//#include "LiquidCrystal_I2C.h"

#define pin_PIR 26
#define pin_Flujo 13
#define pin_boton 14
#define pin_LED 25
#define pin_DHT 17
#define pin_bom 16
#define pin_rele2 27

//LiquidCrystal_I2C lcd(0x27,16,2);
int bot, uBot = LOW;
float temp = 0, flu, hum = 0;
long previousMillis = 0;
long currentMillis = 0;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
unsigned long lTimer = 0;
unsigned long timer = 0;
unsigned long timer1 = 0, timer2 = 0, bTimer = 0;
String tMaq = "";
DHT dht(pin_DHT, DHT11);
bool uPIR = false, blink = false, led = false;
unsigned int visitas = 0;

bool lecturaPIR() {
  int pir = digitalRead(pin_PIR);
  if (pir == HIGH) {
    return true;
  }
  if (pir == LOW) {
    return false;
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
  temp = dht.readTemperature();
  hum = dht.readHumidity();
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
      if ((millis() - timer) > 10000) {
        Serial.println("\n no se pudo conectar a la red");
        blink = true;
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

  dht.begin();
  pinMode(pin_bom, OUTPUT);  //bomba
  digitalWrite(pin_bom, HIGH);
  pinMode(pin_rele2, OUTPUT);  //bombaG/humidificador
  digitalWrite(pin_rele2, HIGH);
  pinMode(pin_LED, OUTPUT);            //led
  pinMode(pin_PIR, INPUT_PULLDOWN);    //pir
  pinMode(pin_boton, INPUT_PULLDOWN);  //boton
  pinMode(pin_Flujo, INPUT);           //flujo
  attachInterrupt(digitalPinToInterrupt(pin_Flujo), pulseCounter, FALLING);
  //esp_sleep_enable_timer_wakeup(1000000 * 3600 * 13);
}

void loop() {
  //mediciones de sensores y lectura del boton
  mediciones();
  bot = digitalRead(pin_boton);
  //cuando se pulsa el boton
  if (bot == HIGH && uBot == LOW) {
    digitalWrite(pin_bom, LOW);  //enciende la bomba
    if (tMaq.equalsIgnoreCase("Atrapanieblas")) {
      digitalWrite(pin_rele2, LOW);  //humidificador
    }
    bTimer = millis();
    uBot = HIGH;
    Serial.println("boton");
  }

  if (millis() - bTimer > 7500) {
    digitalWrite(pin_rele2, HIGH);  //apaga segundo rele bomba o humidificador
  }

  if (lecturaPIR()) {
    Serial.println("persona");
    Serial.println(temp);
  }
  //una vez pulsado el boton y detectando personas registra datos
  if (lecturaPIR() && uBot == HIGH) {
    if (uPIR = false) {
      visitas++;
    }
    uPIR = true;
    timer1 = millis();
    timer2 = millis();
    if (millis() > timer2 + 1500) {
      mediciones();
      registrarDatos(temp, hum, flu, visitas);
      timer2 = millis();
    }
  }

  if (!lecturaPIR() && uPIR) {
    uPIR = false;
  }

  if (uBot == HIGH && millis() - bTimer > 5000) {
    digitalWrite(pin_bom, HIGH);  //apaga la primera bomba
    if (tMaq.equalsIgnoreCase("Aguas_grises")) {
      digitalWrite(pin_rele2, LOW);  //enciende la segunda bomba
    }
    if (millis() - bTimer > 15000) {
      uBot = LOW;
      Serial.println("Cooldown");
    }
  }
  //subir datos al final del dia
  if (String(rtc.now().timestamp(DateTime::TIMESTAMP_TIME)).equals("23:59:59")) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Subiendo_datos");
      revArch("/", tMaq);
      blink = false;
    } else {
      blink = true;
    }
    visitas = 0;
    //esp_deep_sleep_start();
  }
  //parpadeo del led
  if (blink) {
    if (millis() - lTimer > 1000) {
      if (led) {
        digitalWrite(pin_LED, LOW);
      } else {
        digitalWrite(pin_LED, HIGH);
      }
      led = !led;
      lTimer = millis();
    }
  } else {
    digitalWrite(pin_LED, HIGH);
  }
}
