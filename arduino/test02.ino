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

// Modo manual: botón de modo + joystick analógico
const int PIN_BOTON_MODO = 7;
const int PIN_JOYSTICK_X = A0;
const int PIN_JOYSTICK_SW = 12;

// Cambiar a true si el LED RGB es ánodo común
const bool RGB_ANODO_COMUN = false;

// Ángulos de la plataforma (tachos)
const int ANGULO_PLASTICO = 0;
const int ANGULO_PAPEL = 90;
const int ANGULO_METAL = 180;
const int ANGULO_CENTRAL = 90;  // Reposo: basura cae al centro

const int ANGULO_MIN_PLATAFORMA = 0;
const int ANGULO_MAX_PLATAFORMA = 180;

// Cambiar a true si el joystick mueve la plataforma al revés
const bool INVERTIR_JOYSTICK = false;

// Ángulos de la compuerta (ajustar según el mecanismo)
const int GATE_CLOSED = 80;
const int GATE_OPEN = 0;

// Tiempos de movimiento (ms)
const int PASO_PLATAFORMA = 2;           // grados por paso (menor = más suave)
const int RETARDO_PASO_PLATAFORMA = 40;  // ms entre pasos (mayor = más lento)
const int TIEMPO_MOVIMIENTO_COMPUERTA = 120;  // ms para que el servo de compuerta llegue
const int TIEMPO_COMPUERTA_ABIERTA = 1500;
const int INTERVALO_PARPADEO_ANALISIS = 300;

// Joystick manual
const int JOYSTICK_CENTRO = 512;
const int JOYSTICK_ZONA_MUERTA = 120;
const int PASO_MANUAL = 3;
const int RETARDO_PASO_MANUAL = 35;
const int DEBOUNCE_BOTON_MS = 50;
const unsigned long TIMEOUT_PULSEIN_US = 30000;  // Evita bloquear ~1 s sin eco
const int INTERVALO_ULTRASONIDO = 80;            // ms entre lecturas del HC-SR04

// Distancia mínima para detectar objeto (cm)
const int distanciaDeteccion = 10;

bool esperandoRespuesta = false;
bool modoManual = false;
int anguloPlataformaActual = ANGULO_CENTRAL;
int anguloCompuertaActual = GATE_CLOSED;
int ultimoAnguloMostrado = -1;
unsigned long ultimoParpadeo = 0;
unsigned long ultimaMedicionUltrasonido = 0;
bool ledAnalisisEncendido = true;

void moverPlataformaLento(int anguloDestino);
void moverCompuertaRapida(int anguloDestino);
void volverAReady();

int limitarAnguloPlataforma(int angulo) {
  if (angulo < ANGULO_MIN_PLATAFORMA) {
    return ANGULO_MIN_PLATAFORMA;
  }
  if (angulo > ANGULO_MAX_PLATAFORMA) {
    return ANGULO_MAX_PLATAFORMA;
  }
  return angulo;
}

void escribirPlataforma(int angulo) {
  angulo = limitarAnguloPlataforma(angulo);
  platformServo.write(angulo);
  anguloPlataformaActual = angulo;
}

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

void setColorManual() {
  setRgb(true, false, true);  // Magenta
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

void beepModoManual() {
  emitirTono(900, 100);
  delay(50);
  emitirTono(900, 100);
}

void beepModoAutomatico() {
  emitirTono(700, 150);
}

bool botonPresionado(int pin, int &estadoEstable, int &estadoAnterior, unsigned long &ultimoDebounce, bool &presionadoActivo) {
  int lectura = digitalRead(pin);
  bool presionado = false;

  if (lectura != estadoAnterior) {
    ultimoDebounce = millis();
    estadoAnterior = lectura;
  }

  if ((millis() - ultimoDebounce) > DEBOUNCE_BOTON_MS) {
    if (lectura != estadoEstable) {
      estadoEstable = lectura;
    }

    if (estadoEstable == LOW && !presionadoActivo) {
      presionado = true;
      presionadoActivo = true;
    }

    if (estadoEstable == HIGH) {
      presionadoActivo = false;
    }
  }

  return presionado;
}

void mostrarPantallaManual() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MANUAL ");
  lcd.print(ANGULO_MIN_PLATAFORMA);
  lcd.print("-");
  lcd.print(ANGULO_MAX_PLATAFORMA);
  lcd.setCursor(0, 1);
  lcd.print("Ang:");
  lcd.print(anguloPlataformaActual);
  lcd.print("        ");
  ultimoAnguloMostrado = anguloPlataformaActual;
}

void actualizarAnguloEnPantalla() {
  if (anguloPlataformaActual != ultimoAnguloMostrado) {
    lcd.setCursor(0, 1);
    lcd.print("Ang:");
    lcd.print(anguloPlataformaActual);

    if (anguloPlataformaActual <= ANGULO_MIN_PLATAFORMA) {
      lcd.print(" MIN   ");
    } else if (anguloPlataformaActual >= ANGULO_MAX_PLATAFORMA) {
      lcd.print(" MAX   ");
    } else {
      lcd.print("       ");
    }

    ultimoAnguloMostrado = anguloPlataformaActual;
  }
}

void entrarModoManual() {
  modoManual = true;
  esperandoRespuesta = false;

  setColorManual();
  beepModoManual();
  mostrarPantallaManual();
}

void salirModoManual() {
  modoManual = false;
  volverAReady();
  beepModoAutomatico();
}

void procesarBotonModo() {
  static int estadoEstable = HIGH;
  static int estadoAnterior = HIGH;
  static unsigned long ultimoDebounce = 0;
  static bool presionadoActivo = false;

  if (botonPresionado(PIN_BOTON_MODO, estadoEstable, estadoAnterior, ultimoDebounce, presionadoActivo)) {
    if (modoManual) {
      salirModoManual();
    } else {
      entrarModoManual();
    }
  }
}

void controlarPlataformaJoystick() {
  int lecturaX = analogRead(PIN_JOYSTICK_X);
  bool moverIzquierda = lecturaX < (JOYSTICK_CENTRO - JOYSTICK_ZONA_MUERTA);
  bool moverDerecha = lecturaX > (JOYSTICK_CENTRO + JOYSTICK_ZONA_MUERTA);

  if (INVERTIR_JOYSTICK) {
    bool temp = moverIzquierda;
    moverIzquierda = moverDerecha;
    moverDerecha = temp;
  }

  if (moverIzquierda) {
    int nuevoAngulo = limitarAnguloPlataforma(anguloPlataformaActual - PASO_MANUAL);
    if (nuevoAngulo != anguloPlataformaActual) {
      escribirPlataforma(nuevoAngulo);
      delay(RETARDO_PASO_MANUAL);
    }
  } else if (moverDerecha) {
    int nuevoAngulo = limitarAnguloPlataforma(anguloPlataformaActual + PASO_MANUAL);
    if (nuevoAngulo != anguloPlataformaActual) {
      escribirPlataforma(nuevoAngulo);
      delay(RETARDO_PASO_MANUAL);
    }
  }

  actualizarAnguloEnPantalla();
}

void abrirCompuertaManual() {
  lcd.setCursor(0, 1);
  lcd.print("Compuerta ON   ");

  moverCompuertaRapida(GATE_OPEN);
  delay(TIEMPO_COMPUERTA_ABIERTA);
  moverCompuertaRapida(GATE_CLOSED);

  lcd.setCursor(0, 1);
  lcd.print("Ang:");
  lcd.print(anguloPlataformaActual);
  lcd.print("       ");
  ultimoAnguloMostrado = anguloPlataformaActual;
}

void actualizarModoManual() {
  controlarPlataformaJoystick();

  static int estadoEstable = HIGH;
  static int estadoAnterior = HIGH;
  static unsigned long ultimoDebounce = 0;
  static bool presionadoActivo = false;

  if (botonPresionado(PIN_JOYSTICK_SW, estadoEstable, estadoAnterior, ultimoDebounce, presionadoActivo)) {
    abrirCompuertaManual();
  }
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

  pinMode(PIN_BOTON_MODO, INPUT_PULLUP);
  pinMode(PIN_JOYSTICK_SW, INPUT_PULLUP);

  platformServo.attach(platformServoPin);
  gateServo.attach(gateServoPin);

  escribirPlataforma(ANGULO_CENTRAL);
  gateServo.write(GATE_CLOSED);
  anguloCompuertaActual = GATE_CLOSED;

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");

  setColorListo();
}

void loop() {
  procesarBotonModo();

  if (modoManual) {
    actualizarModoManual();
    procesarBotonModo();
    return;
  }

  actualizarLedAnalizando();

  if (!esperandoRespuesta) {
    unsigned long ahora = millis();
    if (ahora - ultimaMedicionUltrasonido >= INTERVALO_ULTRASONIDO) {
      ultimaMedicionUltrasonido = ahora;
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
  }

  if (Serial.available() > 0) {
    char tipo = Serial.read();
    procesarRespuesta(tipo);
  }

  procesarBotonModo();
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
  lcd.print("Tipo: ");
  lcd.print(tipo);
  lcd.print("        ");  // Limpia restos de textos anteriores
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void moverPlataformaLento(int anguloDestino) {
  anguloDestino = limitarAnguloPlataforma(anguloDestino);

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

  escribirPlataforma(anguloDestino);
}

void moverCompuertaRapida(int anguloDestino) {
  if (anguloDestino == anguloCompuertaActual) {
    return;
  }

  gateServo.write(anguloDestino);
  anguloCompuertaActual = anguloDestino;
  delay(TIEMPO_MOVIMIENTO_COMPUERTA);
}

void depositarBasura(int anguloPlataforma) {
  setColorDeposito();

  moverPlataformaLento(anguloPlataforma);

  lcd.setCursor(0, 1);
  lcd.print("Depositando...");

  moverCompuertaRapida(GATE_OPEN);
  delay(TIEMPO_COMPUERTA_ABIERTA);

  moverCompuertaRapida(GATE_CLOSED);

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

  long duracion = pulseIn(echoPin, HIGH, TIMEOUT_PULSEIN_US);
  if (duracion == 0) {
    return -1;
  }

  long distancia = duracion * 0.0343 / 2;

  return distancia;
}
