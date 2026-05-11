# Actividad2_Instrumentacion_electronica
Actividad 2 de la asignatura instrumentación electrónica con título Desarrollo de lógica de control y actuación para ascensor inteligente en entorno industrial ACME S.A.

INTRODUCCIÓN Y ENLACES AL PROYECTO

Este proyecto representa el funcionamiento de un ascensor de la empresa ACME S.A.. 

Hemos diseñado un sistema que gestiona un ascensor de cinco plantas, detecta la presencia de usuarios y controla automáticamente la temperatura del entorno.  Simulación en Wokwi: https://wokwi.com/projects/463751057403112449 Repositorio en GitHub: https://github.com/amoresdaniel/Actividad2_Instrumentacion_electronica.git

ANÁLISIS Y ADAPTACIÓN DE EJEMPLOS 

Para el desarrollo de la práctica, se han analizado y probado los modelos técnicos proporcionados en los contenidos de la asignatura, adaptándolos a los requisitos solicitados en la práctica 

Lógica de control térmico: Se ha integrado un algoritmo de tres posiciones con zona muerta. Se tomó como base el modelo teórico que define límites máximos y mínimos de control (+/- 3 grados) alrededor de un punto de referencia. En nuestro programa, hemos ajustado este margen a 2 grados desde los 25 grados.  

Gestión de actuadores (Servomotor): Se ha utilizado la lógica del ejemplo de servomotores, donde se incluye la librería Servo.h y se mapean ángulos específicos para representar posiciones físicas. En lugar de un barrido continuo, hemos adaptado el código para que el servo viaje a ángulos fijos (0º, 45º, 90º, 135º, 180º) que representan las cinco plantas del edificio.  

Detección de presencia mediante interrupciones: Siguiendo las indicaciones del tema 6 sobre la ineficiencia del polling, se ha implementado una rutina de servicio de interrupción (ISR). Se ha adaptado el ejemplo de interrupción por hardware de Arduino para que el sensor PIR active una señal inmediata en el pin 2 de la placa al detectar movimiento.  

Control por infrarrojos: Se ha integrado la gestión del receptor IR basándose en la identificación de códigos de comando específicos para el mando a distancia.  

DISEÑO DEL HARDWARE E INTEGRACIÓN 

El circuito integra todos los componentes necesarios para el funcionamiento del ascensor y la supervisión ambiental, cuenta como elemento principal con la placa Arduino UNO, a este elemento se le suman diferentes componentes para poder implementar las tareas solicitadas. En primer lugar, utilizamos un 
sensor DHT22 que permite la medición de temperatura y humedad así mismo usamos un sensor PIR para la detección de un usuario.  

El IR Receiver y el IR remote permiten interactuar con el servomotor.

Para representar los datos que obtenemos de los sensores usamos un servomotor para visualizar los movimientos de la cabina y dos LEDs (rojo/azul) que simulan las funciones de calentar y enfriar, también se ha utilizado un LCD 16x2 I2C para mostrar el estado del sistema.  

DESARROLLO DEL PROGRAMA Y EFICIENCIA

El código se ha estructurado de forma modular para que sea comprensible, claro y permita una fácil actualización y reutilización.

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

RESULTADOS Y SUPERVISIÓN 
El sistema ha sido probado satisfactoriamente en el entorno de simulación:

La pantalla LCD muestra claramente la planta actual, si existe presencia en la cabina y el estado del control térmico (ENFRIAR/CALOR/OK). 

El sistema envía a través del puerto serie un log detallado de cada ciclo de operación.

 
 

