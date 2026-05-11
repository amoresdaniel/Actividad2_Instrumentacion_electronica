#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Servo.h>
#include <IRremote.h>

//Definición de pines
#define PIR_PIN 2 //Pin de interrupción para el sensor de presencia
#define DHTPIN 4 //Pin para el sensor DHT22
#define DHTTYPE DHT22
#define PIN_CALENTAR 5 //LED Rojo (Válvula de calor)
#define PIN_ENFRIAR 6 //LED Azul (Válvula de frío)
#define SERVO_PIN 9 //Pin PWM para el Servomotor
#define IR_RECEIVE_PIN 11 //Pin para el receptor Infrarrojo

//Inicializamos sensor, pantalla y servo
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo ascensorServo;

//Declaramos variables del ascensor
int PlantaActual = 1;
int PlantaDestino = 1;
//Ángulos para las plantas 1, 2, 3, 4 y 5
int AngulosPlanta[5] = {0, 45, 90, 135, 180}; 

//Variables para detectar si hay una presencia
volatile bool PresenciaDetectada = false; //volatile permite que no se mantenga el valor booleano
unsigned long TiempoUltimaPresencia = 0;

//Variables para la temperatura
float TempDeseada = 25.0; // Baremo
float Margen = 2.0; 

void setup() {
  Serial.begin(9600);
  
  // Inicialización de componentes
  dht.begin();
  lcd.init();
  lcd.backlight();
  ascensorServo.attach(SERVO_PIN);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Inicia el receptor IR
  
  //Configuración de pines de temperatura
  pinMode(PIN_CALENTAR, OUTPUT);
  pinMode(PIN_ENFRIAR, OUTPUT);
  
  //Configuración de la interrupción para el sensor de presencia
  pinMode(PIR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), isr_Presencia, RISING);
  
  //Posición inicial del ascensor
  ascensorServo.write(AngulosPlanta[0]);
  
  lcd.setCursor(0, 0);
  lcd.print("Ascensor ACME");
  delay(2000);
  lcd.clear();
}

void loop() {
  if (IrReceiver.decode()) {
    int comando = IrReceiver.decodedIRData.command;
    //Códigos estándar del mando de Wokwi para los botones 1 al 5
    if (comando == 48) PlantaDestino = 1;
    else if (comando == 24) PlantaDestino = 2;
    else if (comando == 122) PlantaDestino = 3;
    else if (comando == 16) PlantaDestino = 4;
    else if (comando == 56) PlantaDestino = 5;
    
    IrReceiver.resume();
  }

  if (PlantaActual != PlantaDestino) {
    //Movemos el servo al ángulo correspondiente a la planta (índice es planta - 1)
    ascensorServo.write(AngulosPlanta[PlantaDestino - 1]);
    PlantaActual = PlantaDestino;
  }

  float Temperatura = dht.readTemperature();
  float Humedad = dht.readHumidity();

  float LimMaxControl = TempDeseada + Margen; // 27.0 C
  float LimMinControl = TempDeseada - Margen; // 23.0 C
  String EstadoClima = "OFF";

  if (Temperatura >= LimMaxControl) {
    digitalWrite(PIN_ENFRIAR, HIGH);
    digitalWrite(PIN_CALENTAR, LOW);
    EstadoClima = "ENFRIANDO";
  } 
  else if (Temperatura <= LimMinControl) {
    digitalWrite(PIN_CALENTAR, HIGH);
    digitalWrite(PIN_ENFRIAR, LOW);
    EstadoClima = "CALENTANDO";
  } 
  else {
    //Dentro de la zona de comfort (23ºC - 27ºC)
    digitalWrite(PIN_CALENTAR, LOW);
    digitalWrite(PIN_ENFRIAR, LOW);
    EstadoClima = "OK";
  }

  String EstadoPresencia = "NO";
  //Si la interrupción saltó, actualizamos el temporizador
  if (PresenciaDetectada) {
    TiempoUltimaPresencia = millis();
    PresenciaDetectada = false; // Reseteamos la bandera
  }
  //Mantenemos el estado de presencia activo durante 5 segundos desde el último movimiento
  if (millis() - TiempoUltimaPresencia < 5000) {
    EstadoPresencia = "SI";
  }

  actualizarLCD(PlantaActual, Temperatura, EstadoClima, EstadoPresencia);
  
  Serial.print("Planta: "); Serial.print(PlantaActual);
  Serial.print(" | Presencia: "); Serial.print(EstadoPresencia);
  Serial.print(" | Temp: "); Serial.print(Temperatura); Serial.print(" C");
  Serial.print(" | Hum: "); Serial.print(Humedad); Serial.print(" %");
  Serial.print(" | Clima: "); Serial.println(EstadoClima);
  
  delay(500);
}

//INTERRUPCIÓN
void isr_Presencia() {
  PresenciaDetectada = true;
}

//Actualización de la pantalla
void actualizarLCD(int Planta, float Temp, String Clima, String Presencia) {
  lcd.setCursor(0, 0);
  lcd.print("P:"); lcd.print(Planta);
  lcd.print(" Pres:"); lcd.print(Presencia); lcd.print("   ");
  
  lcd.setCursor(0, 1);
  lcd.print("T:"); lcd.print(Temp, 1);
  lcd.print(" "); lcd.print(Clima); 
  lcd.print("        "); 
}
