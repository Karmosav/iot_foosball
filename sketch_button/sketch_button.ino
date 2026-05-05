#define BUTTON_PIN D6

bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.println("Button test ready");
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Button pressed!");
  }

  lastButtonState = buttonState;

  delay(50); // small debounce delay
}