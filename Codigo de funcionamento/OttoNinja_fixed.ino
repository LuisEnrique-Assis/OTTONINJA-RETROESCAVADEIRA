#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

const char* ssid = "POCO X4 GT";   // nome do seu WiFi
const char* password = "jonny011"; // senha do seu WiFi

#define LEFT_LEG   0
#define RIGHT_LEG  1
#define LEFT_FOOT  2
#define RIGHT_FOOT 3

#define TRIG_PIN 18
#define ECHO_PIN 19
#define BUZZER_PIN 4

WiFiServer server(80);

bool walkMode = true;   // começa em modo caminhada (bate com o "mode" inicial do site)
bool rollMode = false;
int roll_right_forward_speed = 40;
int roll_left_forward_speed = 40;
int roll_right_backward_speed = 40;
int roll_left_backward_speed = 40;
String command = "";

#define SERVO_MIN 120
#define SERVO_MAX 500

void servoWrite(uint8_t canal, uint8_t angulo) {
  angulo = constrain(angulo, 0, 180);
  uint16_t pulso = map(angulo, 0, 180, SERVO_MIN, SERVO_MAX);
  pwm.setPWM(canal, 0, pulso);
}

long ultrasound_distance_1() {
  long duration, distance;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout de 30ms evita travar se não houver eco
  distance = duration / 58;
  return distance;
}

void beepESP(uint8_t pin, int frequency, int duration_ms) {
  int halfPeriod = 1000000 / frequency / 2;
  long cycles = (long)frequency * duration_ms / 1000;
  for (long i = 0; i < cycles; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(pin, LOW);
    delayMicroseconds(halfPeriod);
  }
}

void AvoidanceWalk() {
  if (ultrasound_distance_1() <= 15) {
    Home();
    delay(50);
    WalkRight();
  }
  WalkForward();
}

void AvoidanceRoll() {
  if (ultrasound_distance_1() <= 15) {
    NinjaRollStop();
    NinjaRollRight(roll_right_forward_speed, roll_left_forward_speed);
  }
  NinjaRollForward(roll_right_forward_speed, roll_left_forward_speed);
}

void Home() {
  servoWrite(LEFT_FOOT, 90);
  servoWrite(RIGHT_FOOT, 90);
  servoWrite(LEFT_LEG, 60);
  servoWrite(RIGHT_LEG, 120);
  delay(100);
}

void SetWalk() {
  walkMode = true;
  rollMode = false;
  servoWrite(LEFT_LEG, 80);
  servoWrite(RIGHT_LEG, 160);
  delay(100);
}

void SetRoll() {
  walkMode = false;
  rollMode = true;
  servoWrite(LEFT_LEG, 120);
  servoWrite(RIGHT_LEG, 10);
  delay(100);
}

void TiltToRight() {
  servoWrite(LEFT_LEG, 0);
  servoWrite(RIGHT_LEG, 65);
  delay(300);
}

void TiltToLeft() {
  servoWrite(LEFT_LEG, 120);
  servoWrite(RIGHT_LEG, 180);
  delay(300);
}

void MoveRightFoot(int s, int t) {
  servoWrite(RIGHT_FOOT, s);
  delay(t);
  servoWrite(RIGHT_FOOT, 90);
  delay(100);
}

void MoveLeftFoot(int s, int t) {
  servoWrite(LEFT_FOOT, s);
  delay(t);
  servoWrite(LEFT_FOOT, 90);
  delay(100);
}

void ReturnFromRight() {
  servoWrite(LEFT_LEG, 60);
  for (int count = 65; count <= 120; count += 2) {
    servoWrite(RIGHT_LEG, count);
    delay(25);
  }
  delay(300);
}

void ReturnFromLeft() {
  servoWrite(RIGHT_LEG, 120);
  for (int count = 120; count >= 60; count -= 2) {
    servoWrite(LEFT_LEG, count);
    delay(25);
  }
  delay(300);
}

void WalkForward() {
  TiltToRight();
  delay(100);
  MoveRightFoot(70, 250);
  delay(100);
  ReturnFromRight();

  TiltToLeft();
  delay(100);
  MoveLeftFoot(140, 350);
  delay(100);
  ReturnFromLeft();
}

void WalkRight() {
  TiltToRight();
  delay(100);
  MoveRightFoot(70, 350);
  delay(100);
  ReturnFromRight();
}

void WalkBackward() {
  TiltToRight();
  delay(100);
  MoveRightFoot(120, 250);
  delay(100);
  ReturnFromRight();

  TiltToLeft();
  delay(100);
  MoveLeftFoot(60, 350);
  delay(100);
  ReturnFromLeft();
}

void WalkLeft() {
  TiltToLeft();
  delay(100);
  MoveLeftFoot(140, 350);
  delay(100);
  ReturnFromLeft();
}

void NinjaRollForward(int speedR, int speedL) {
  servoWrite(LEFT_FOOT, 90 + speedL);
  servoWrite(RIGHT_FOOT, 90 - speedR);
}

void NinjaRollRight(int speedR, int speedL) {
  servoWrite(LEFT_FOOT, 90 + speedL);
  servoWrite(RIGHT_FOOT, 90 + speedR);
}

void NinjaRollLeft(int speedR, int speedL) {
  servoWrite(LEFT_FOOT, 90 - speedL);
  servoWrite(RIGHT_FOOT, 90 - speedR);
}

void NinjaRollBackward(int speedR, int speedL) {
  servoWrite(LEFT_FOOT, 90 - speedL);
  servoWrite(RIGHT_FOOT, 90 + speedR);
}

void NinjaRollStop() {
  servoWrite(LEFT_FOOT, 90);
  servoWrite(RIGHT_FOOT, 90);
}

void joystickRoll(int x, int y) {
  if ((x >= -5) && (x <= 5) && (y >= -5) && (y <= 5)) {
    NinjaRollStop();
  } else {
    int LWS = map(y, -50, 50, 135, 45);
    int RWS = map(y, -50, 50, 45, 135);
    int LWD = map(x, 50, -50, 45, 0);
    int RWD = map(x, 50, -50, 0, -45);

    servoWrite(LEFT_FOOT, LWS + LWD);
    servoWrite(RIGHT_FOOT, RWS + RWD);
  }
}

void decodeSpeeds(String c) {
  int counter = 0;
  String RF = "", LF = "", RB = "", LB = "";

  for (int i = 1; i < c.length(); i++) {
    if (isDigit(c[i])) {
      if (counter == 0) RF += c[i];
      else if (counter == 1) LF += c[i];
      else if (counter == 2) RB += c[i];
      else if (counter == 3) LB += c[i];
    } else if (c[i] == '&') {
      counter++;
    }
  }

  roll_right_forward_speed = RF.toInt();
  roll_left_forward_speed = LF.toInt();
  roll_right_backward_speed = RB.toInt();
  roll_left_backward_speed = LB.toInt();
}

void GetCoords(String str) {
  // A URL enviada pelo ninja.html é do tipo: /Jx,y HTTP/1.1
  // O 'H' de HTTP/1.1 serve de fim natural para o valor de y.
  String x = str.substring(str.lastIndexOf('J') + 1, str.lastIndexOf(','));
  String y = str.substring(str.lastIndexOf(',') + 1, str.lastIndexOf('H') - 1);

  if (walkMode) {
    // o ninja.html só usa joystick no modo roll, mas deixamos preparado
    joystickRoll(x.toInt(), y.toInt());
  } else {
    joystickRoll(x.toInt(), y.toInt());
  }
}

void CheckClient() {
  WiFiClient client = server.available();
  if (!client) return;

  client.setTimeout(200); // evita travar o loop esperando dados que nunca chegam
  unsigned long start = millis();
  while (!client.available()) {
    if (millis() - start > 200) {
      client.stop();
      return;
    }
    delay(1);
  }

  String req = client.readStringUntil('\r');
  client.flush();

  if (req.indexOf("/walkmode") != -1)      { command = "walkmode"; }
  else if (req.indexOf("/rollmode") != -1) { command = "rollmode"; }
  else if (req.indexOf("/home") != -1)     { command = "home"; }
  else if (req.indexOf("/forward") != -1)  { command = "forward"; }
  else if (req.indexOf("/right") != -1)    { command = "right"; }
  else if (req.indexOf("/backward") != -1) { command = "backward"; }
  else if (req.indexOf("/left") != -1)     { command = "left"; }
  else if (req.indexOf("/stop") != -1)     { command = "stop"; }
  else if (req.indexOf("/avoidancewalk") != -1) { command = "avoidancewalk"; }
  else if (req.indexOf("/avoidanceroll") != -1) { command = "avoidanceroll"; }
  else if (req.indexOf("/R=") != -1) {
    decodeSpeeds(req);
    command = "";
  }
  else if (req.indexOf("/J") != -1) {
    GetCoords(req);
    command = "";
  }
  else {
    client.stop();
    return;
  }

  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nAccess-Control-Allow-Origin: *\r\n\r\nOK";
  client.print(s);
  delay(1);
  client.stop();
}

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  server.begin();
  Serial.print("Use este IP no ninja.html (com barra no final), ex: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  Wire.begin(21, 22);
  pwm.begin();
  pwm.setPWMFreq(50);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  beepESP(BUZZER_PIN, 440, 500);
  Home();
}

void loop() {
  CheckClient(); // Verifica comandos vindos do ninja.html via WiFi

  if (command == "walkmode") {
    SetWalk();
    command = "";
  }
  else if (command == "rollmode") {
    SetRoll();
    command = "";
  }
  else if (command == "home") {
    Home();
    command = "";
  }
  else if (command == "forward") {
    if (walkMode) WalkForward();
    else NinjaRollForward(roll_right_forward_speed, roll_left_forward_speed);
    command = "";
  }
  else if (command == "backward") {
    if (walkMode) WalkBackward();
    else NinjaRollBackward(roll_right_backward_speed, roll_left_backward_speed);
    command = "";
  }
  else if (command == "right") {
    if (walkMode) WalkRight();
    else NinjaRollRight(roll_right_forward_speed, roll_left_forward_speed);
    command = "";
  }
  else if (command == "left") {
    if (walkMode) WalkLeft();
    else NinjaRollLeft(roll_right_backward_speed, roll_left_backward_speed);
    command = "";
  }
  else if (command == "stop") {
    NinjaRollStop();
    command = "";
  }
  else if (command == "avoidancewalk") {
    AvoidanceWalk();
  }
  else if (command == "avoidanceroll") {
    AvoidanceRoll();
  }
}
