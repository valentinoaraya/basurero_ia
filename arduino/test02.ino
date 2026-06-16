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

// LED RGB (cátodo común) y buzzer pasivo
const int PIN_RGB_R = 2;
const int PIN_RGB_G = 3;
const int PIN_RGB_B = 4;
const int PIN_BUZZER = 8;

// Cambiar a true si el LED RGB es ánodo común
const bool RGB_ANODO_COMUN = false;

// Ángulos de la plataforma (tachos)
const int ANGULO_PLASTICO = 0;
const int ANGULO_PAPEL = 90;
const int ANGULO_METAL = 180;
const int ANGULO_CENTRAL = ANGULO_PAPEL;  // Posición de reposo tras depositar

// Ángulos de la compuerta (ajustar según el mecanismo)
const int GATE_CLOSED = 0;
const int GATE_OPEN = 90;

// Tiempos de movimiento (ms)
const int PASO_PLATAFORMA = 2;           // grados por paso (menor = más suave)
const int RETARDO_PASO_PLATAFORMA = 40;  // ms entre pasos (mayor = más lento)
const int TIEMPO_COMPUERTA_ABIERTA = 1500;
const int TIEMPO_CIERRE_COMPUERTA = 500;
const int INTERVALO_PARPADEO_ANALISIS = 300;

// Distancia mínima para detectar objeto (cm)
const int distanciaDeteccion = 10;

bool esperandoRespuesta = false;
int anguloPlataformaActual = ANGULO_CENTRAL;
unsigned long ultimoParpadeo = 0;
bool ledAnalisisEncendido = true;

void setRgb(bool rojo, bool verde, bool azul) {
  if (RGB_ANODO_COMUN) {
    digitalWrite(PIN_RGB_R, rojo ? LOW : HIGH);
    digitalWrite(PIN_RGB_G, verde ? LOW : HIGH);
    digitalWrite(PIN_RGB_B, azul ? LOW : HIGH);
  } else {
    digitalWrite(PIN_RGB_R, rojo ? HIGH : LOW);
    digitalWrite(PIN_RGB_G, verde ? HIGH : LOW);
    digitalWrite(PIN_RGB_B, azul ? HIGH : LOW);
  }
}

void apagarRgb() {
  setRgb(false, false, false);
}

void setColorListo() {
  setRgb(false, true, false);  // Verde
}

void setColorAdvertencia() {
  setRgb(true, true, false);  // Amarillo
}

void setColorAnalizando() {
  setRgb(false, false, true);  // Azul
}

void setColorDeposito() {
  setRgb(false, true, false);  // Verde
}

void setColorError() {
  setRgb(true, false, false);  // Rojo
}

void emitirTono(int frecuencia, int duracionMs) {
  tone(PIN_BUZZER, frecuencia, duracionMs);
  delay(duracionMs);
  noTone(PIN_BUZZER);
}

void beepDeteccion() {
  emitirTono(1000, 120);
}

void beepPlastico() {
  emitirTono(1200, 150);
  delay(60);
  emitirTono(1200, 150);
}

void beepPapel() {
  emitirTono(800, 250);
}

void beepMetal() {
  for (int i = 0; i < 3; i++) {
    emitirTono(600, 100);
    delay(70);
  }
}

void beepError() {
  emitirTono(400, 500);
}

void actualizarLedAnalizando() {
  if (!esperandoRespuesta) {
    return;
  }

  unsigned long ahora = millis();
  if (ahora - ultimoParpadeo >= INTERVALO_PARPADEO_ANALISIS) {
    ultimoParpadeo = ahora;
    ledAnalisisEncendido = !ledAnalisisEncendido;

    if (ledAnalisisEncendido) {
      setColorAnalizando();
    } else {
      apagarRgb();
    }
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(PIN_RGB_R, OUTPUT);
  pinMode(PIN_RGB_G, OUTPUT);
  pinMode(PIN_RGB_B, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  platformServo.attach(platformServoPin);
  gateServo.attach(gateServoPin);

  platformServo.write(ANGULO_CENTRAL);
  anguloPlataformaActual = ANGULO_CENTRAL;
  gateServo.write(GATE_CLOSED);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");

  setColorListo();
}

void loop() {
  actualizarLedAnalizando();

  if (!esperandoRespuesta) {
    long distancia = medirDistancia();

    if (distancia > 0 && distancia <= distanciaDeteccion) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Objeto detect.");

      lcd.setCursor(0, 1);
      lcd.print("Analizando...");

      setColorAdvertencia();
      beepDeteccion();

      Serial.println("START");
      esperandoRespuesta = true;
      ultimoParpadeo = millis();
      ledAnalisisEncendido = true;
      setColorAnalizando();

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
      beepPlastico();
      depositarBasura(ANGULO_PLASTICO);
      break;

    case 'L':
      mostrarTipo("PAPEL");
      beepPapel();
      depositarBasura(ANGULO_PAPEL);
      break;

    case 'M':
      mostrarTipo("METAL");
      beepMetal();
      depositarBasura(ANGULO_METAL);
      break;

    case 'N':
      setColorError();
      beepError();
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
  setColorDeposito();

  moverPlataformaLento(anguloPlataforma);

  lcd.setCursor(0, 1);
  lcd.print("Depositando...");

  gateServo.write(GATE_OPEN);
  delay(TIEMPO_COMPUERTA_ABIERTA);

  gateServo.write(GATE_CLOSED);
  delay(TIEMPO_CIERRE_COMPUERTA);

  lcd.setCursor(0, 1);
  lcd.print("Volviendo...   ");
  moverPlataformaLento(ANGULO_CENTRAL);

  volverAReady();
}

void volverAReady() {
  esperandoRespuesta = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");

  setColorListo();
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
