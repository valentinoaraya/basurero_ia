void setup() {
  Serial.begin(9600);
  pinMode(4, OUTPUT); // Metal
  pinMode(5, OUTPUT); // Papel
  pinMode(6, OUTPUT); // Plastico
}

void loop() {
  if (Serial.available() > 0) {
    
    char data = Serial.read();
    Serial.print(data);
    
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);

    if (data == 'M') {
      digitalWrite(4, HIGH);
      Serial.println("Detectado: METAL");
    } 
    else if (data == 'L') { // L de Leaf (Papel)
      digitalWrite(5, HIGH);
      Serial.println("Detectado: PAPEL");
    } 
    else if (data == 'P') {
      digitalWrite(6, HIGH);
      Serial.println("Detectado: PLASTICO");
    }
    
    delay(2000); // El LED se queda prendido 2 segundos
    digitalWrite(4, LOW);
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);
  }
}