#include "DHT.h"
#include "DataLogger.h"
#include "WiFi.h"
#define pin_PIR 12
#define pin_Flujo 13

float temp,flu,hum;

long previousMillis = 0;
long currentMillis = 0;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec=0;
long timer = 0;

DHT dht(17,DHT11);

bool uPIR=false;

unsigned int visitas=0;

bool lecturaPIR(){
  int pir=digitalRead(pin_PIR);
  if(pir==HIGH){
    return true;
  }else{
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

void mediciones(){
temp=dht.readTemperature();
hum=dht.readHumidity();
flujo();
}

void setup() {
  File myFile;
  Serial.begin(115200);
 if(!setSD()){
  Serial.println("SD no encontrada");
 }
 if(!setReloj()){
  Serial.println("Reloj no encontrado");
 }
 if(!setRegistros()){
  Serial.println("Error en los registros");
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

  dht.begin();

  pinMode(pin_PIR,INPUT);

  pinMode(pin_Flujo, INPUT);
  attachInterrupt(digitalPinToInterrupt(pin_Flujo), pulseCounter, FALLING);

}

void loop() {
  // put your main code here, to run repeatedly:
  long timer1,timer2;
  mediciones();
  if(lecturaPIR()&& !uPIR){
    visitas++;
    uPIR=true;
    registrarDatos(temp,hum,flu,visitas);
    timer1=millis();
    timer2=millis();
    do{
      if(millis()>timer2+10000){
        mediciones();
        registrarDatos(temp,hum,flu,visitas);
        timer2=millis();
      }
      if(timer1-millis()>60000){
        break;
      }
    }while(1);
    mediciones();
    registrarDatos(temp,hum,flu,visitas);
  }
  if(!lecturaPIR()&& uPIR){
    uPIR=false;
  }
  if(rtc.now().hour()>=18){
    if(WiFi.status()==WL_CONNECTED){
      //subir datos
    }else{
      //lista de espera
    }
    
  }
}