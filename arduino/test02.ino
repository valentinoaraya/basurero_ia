#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo platformServo;
Servo gateServo;

// HC-SR04
const int trigPin = 9;
const int echoPin = 10;

// Servos
const int platformServoPin = 6;
const int gateServoPin = 5;

// Ángulos de la plataforma (tachos)
const int ANGULO_PLASTICO = 0;
const int ANGULO_PAPEL = 90;
const int ANGULO_METAL = 180;

// Ángulos de la compuerta (ajustar según el mecanismo)
const int GATE_CLOSED = 0;
const int GATE_OPEN = 90;

// Tiempos de movimiento (ms)
const int PASO_PLATAFORMA = 2;           // grados por paso (menor = más suave)
const int RETARDO_PASO_PLATAFORMA = 40;  // ms entre pasos (mayor = más lento)
const int TIEMPO_COMPUERTA_ABIERTA = 1500;
const int TIEMPO_CIERRE_COMPUERTA = 500;

// Distancia mínima para detectar objeto (cm)
const int distanciaDeteccion = 10;

bool esperandoRespuesta = false;
int anguloPlataformaActual = ANGULO_PAPEL;

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  platformServo.attach(platformServoPin);
  gateServo.attach(gateServoPin);

  platformServo.write(ANGULO_PAPEL);
  anguloPlataformaActual = ANGULO_PAPEL;
  gateServo.write(GATE_CLOSED);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");
}

void loop() {
  if (!esperandoRespuesta) {
    long distancia = medirDistancia();

    if (distancia > 0 && distancia <= distanciaDeteccion) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Objeto detect.");

      lcd.setCursor(0, 1);
      lcd.print("Analizando...");

      Serial.println("START");
      esperandoRespuesta = true;

      delay(1000);
    }
  }

  if (Serial.available() > 0) {
    char tipo = Serial.read();
    procesarRespuesta(tipo);
  }
}

void procesarRespuesta(char tipo) {
  lcd.clear();

  switch (tipo) {
    case 'P':
      mostrarTipo("PLASTICO");
      depositarBasura(ANGULO_PLASTICO);
      break;

    case 'L':
      mostrarTipo("PAPEL");
      depositarBasura(ANGULO_PAPEL);
      break;

    case 'M':
      mostrarTipo("METAL");
      depositarBasura(ANGULO_METAL);
      break;

    case 'N':
      lcd.setCursor(0, 0);
      lcd.print("Sin deteccion");
      lcd.setCursor(0, 1);
      lcd.print("Reintentar...");
      delay(2000);
      volverAReady();
      break;

    default:
      while (Serial.available() > 0) Serial.read();
      break;
  }
}

void mostrarTipo(const char *tipo) {
  lcd.setCursor(0, 0);
  lcd.print("Tipo:");
  lcd.setCursor(0, 1);
  lcd.print(tipo);
}

void moverPlataformaLento(int anguloDestino) {
  if (anguloDestino == anguloPlataformaActual) {
    return;
  }

  if (anguloDestino > anguloPlataformaActual) {
    for (int angulo = anguloPlataformaActual; angulo <= anguloDestino; angulo += PASO_PLATAFORMA) {
      platformServo.write(angulo);
      delay(RETARDO_PASO_PLATAFORMA);
    }
  } else {
    for (int angulo = anguloPlataformaActual; angulo >= anguloDestino; angulo -= PASO_PLATAFORMA) {
      platformServo.write(angulo);
      delay(RETARDO_PASO_PLATAFORMA);
    }
  }

  platformServo.write(anguloDestino);
  anguloPlataformaActual = anguloDestino;
}

void depositarBasura(int anguloPlataforma) {
  moverPlataformaLento(anguloPlataforma);

  lcd.setCursor(0, 1);
  lcd.print("Depositando...");

  gateServo.write(GATE_OPEN);
  delay(TIEMPO_COMPUERTA_ABIERTA);

  gateServo.write(GATE_CLOSED);
  delay(TIEMPO_CIERRE_COMPUERTA);

  volverAReady();
}

void volverAReady() {
  esperandoRespuesta = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");
}

long medirDistancia() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duracion = pulseIn(echoPin, HIGH);
  long distancia = duracion * 0.0343 / 2;

  return distancia;
}
