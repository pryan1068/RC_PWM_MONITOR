// ===== 8‑Channel RC PWM Reader with Failsafe + Min/Max Sanity Checks =====
// Arduino Uno/Nano (ATmega328P)

volatile uint16_t pulseWidth[8];
volatile uint32_t riseTime[8];
volatile uint32_t lastPulseTime[8];
volatile bool failsafe[8];
volatile bool sanityBad[8];     // NEW: min/max sanity flags

const uint8_t chPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};

// Sanity thresholds
const uint16_t MIN_US = 900;    // anything below this is invalid
const uint16_t MAX_US = 2100;   // anything above this is invalid

// Failsafe threshold
const uint32_t FAILSAFE_TIMEOUT_US = 40000;   // 40 ms

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 8; i++) {
    pinMode(chPins[i], INPUT);
    failsafe[i] = true;
    sanityBad[i] = true;  // until first valid pulse arrives
  }

  // Enable PCINT for D2–D7
  PCICR |= (1 << PCIE2);
  PCMSK2 |= 0b11111100;

  // Enable PCINT for D8–D9
  PCICR |= (1 << PCIE0);
  PCMSK0 |= 0b00000011;
}

void loop() {
  uint32_t now = micros();

  for (int i = 0; i < 8; i++) {

    // FAILSAFE CHECK
    if ((now - lastPulseTime[i]) > FAILSAFE_TIMEOUT_US) {
      failsafe[i] = true;
    } else {
      failsafe[i] = false;
    }

    // MIN/MAX SANITY CHECK
    uint16_t pw = pulseWidth[i];
    if (pw < MIN_US || pw > MAX_US) {
      sanityBad[i] = true;
    } else {
      sanityBad[i] = false;
    }
  }

  // Print channel status
  for (int i = 0; i < 8; i++) {
    Serial.print("CH");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(pulseWidth[i]);
    Serial.print("us  ");

    if (failsafe[i]) {
      Serial.print("[FAILSAFE] ");
    } else if (sanityBad[i]) {
      Serial.print("[BAD_RANGE] ");
    } else {
      Serial.print("[OK] ");
    }

    Serial.print("  ");
  }
  Serial.println();

  delay(50);
}

// ===== Interrupt for D2–D7 =====
ISR(PCINT2_vect) {
  uint32_t now = micros();
  for (int i = 0; i < 6; i++) {
    uint8_t pin = chPins[i];
    if (digitalRead(pin)) {
      riseTime[i] = now;
    } else {
      pulseWidth[i] = now - riseTime[i];
      lastPulseTime[i] = now;
    }
  }
}

// ===== Interrupt for D8–D9 =====
ISR(PCINT0_vect) {
  uint32_t now = micros();
  for (int i = 6; i < 8; i++) {
    uint8_t pin = chPins[i];
    if (digitalRead(pin)) {
      riseTime[i] = now;
    } else {
      pulseWidth[i] = now - riseTime[i];
      lastPulseTime[i] = now;
    }
  }
}
