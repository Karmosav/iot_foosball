#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ===== WiFi =====
const char* WIFI_SSID = "IOT12";
const char* WIFI_PASS = "iotempire";

// ===== MQTT =====
const char* MQTT_BROKER = "192.168.14.1";
const int   MQTT_PORT   = 1883;

const char* TOPIC_GOAL     = "foosball/goal/tiim2";
const char* TOPIC_CANSCORE = "foosball/canscore";

const char* TOPIC_SCORE     = "foosball/score/tiim2";

const char* CLIENT_ID  = "wemos-goal-node2";

// ===== Ultrasonic sensor pins =====
#define TRIG_PIN D6
#define ECHO_PIN D7
#define BUTTON_PIN_ADD D3
#define BUTTON_PIN_REMOVE D4


// ===== Goal detection =====
#define GOAL_DISTANCE_CM 6.0

bool goalAlreadySent = false;
bool canScore = false;   // default: cannot score until MQTT says "true"
bool lastButtonState_add = HIGH;
bool lastButtonState_remove = HIGH;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  msg.trim();
  msg.toLowerCase();

  Serial.print("Received on ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(msg);

  if (String(topic) == TOPIC_CANSCORE) {
    if (msg == "true") {
      canScore = true;
      Serial.println("Scoring ENABLED");
    } else if (msg == "false") {
      canScore = false;
      goalAlreadySent = false; // reset goal lock
      Serial.println("Scoring DISABLED");
    }
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT... ");

    if (mqttClient.connect(CLIENT_ID)) {
      Serial.println("connected");

      mqttClient.subscribe(TOPIC_CANSCORE);
      Serial.print("Subscribed to: ");
      Serial.println(TOPIC_CANSCORE);

    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 2 seconds");
      delay(2000);
    }
  }
}

float readDistanceCm() {
  long duration;
  float distanceCm;

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1; // no echo
  }

  distanceCm = duration * 0.0343 / 2;
  return distanceCm;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("Wemos D1 mini goal detector starting...");

  connectWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  //button
  pinMode(BUTTON_PIN_ADD, INPUT_PULLUP);
  pinMode(BUTTON_PIN_REMOVE, INPUT_PULLUP);

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  mqttClient.loop();

  float distanceCm = readDistanceCm();

  if (distanceCm == -1) {
    Serial.println("No echo detected");
  } else {
    Serial.print("Distance: ");
    Serial.print(distanceCm);
    Serial.print(" cm - ");

    if (distanceCm < GOAL_DISTANCE_CM) {
      Serial.println("GOAL detected!");

      if (canScore == true) {
        if (!goalAlreadySent) {
          mqttClient.publish(TOPIC_GOAL, "goal");
          Serial.println("MQTT sent: goal");
          goalAlreadySent = true;
        }
      } else {
        Serial.println("Goal ignored because canScore is false");
      }

    } else {
      Serial.println("NO GOAL");

      // Reset so it can detect the next goal
      goalAlreadySent = false;
    }
  }
  bool buttonState_add = digitalRead(BUTTON_PIN_ADD);
  bool buttonState_remove = digitalRead(BUTTON_PIN_REMOVE);

  if (buttonState_add == LOW && lastButtonState_add == HIGH) {
      mqttClient.publish(TOPIC_SCORE, "1");
      Serial.println("added");
  }

  lastButtonState_add = buttonState_add;

  if (buttonState_remove == LOW && lastButtonState_remove == HIGH) {
      mqttClient.publish(TOPIC_SCORE, "-1");
      Serial.println("removed");
  }

  lastButtonState_remove = buttonState_remove;

  delay(300);
}