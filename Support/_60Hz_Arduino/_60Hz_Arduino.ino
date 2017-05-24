void setup() {
  // put your setup code here, to run once:
  pinMode(9, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(9, HIGH);
  delay(16);
  delayMicroseconds(660);
  digitalWrite(9, LOW);
  delay(16);
  delayMicroseconds(660);
}
