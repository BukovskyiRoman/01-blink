#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Display configuration
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;

constexpr uint8_t SCREEN_ADDRESS = 0x3C;

// OLED reset pin is not connected separately
constexpr int OLED_RESET = -1;

constexpr unsigned long UPDATE_INTERVAL = 500;

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);

unsigned long previousUpdate = 0;

constexpr int LED_RED = 2;
constexpr int LED_YELLOW = 16;
constexpr int LED_GREEN = 17;
constexpr int LED_BLUE = 19;
constexpr int LED_WHITE = 18;

constexpr int BUTTON_PIN = 25;

constexpr int POT_PIN = 34; 

// Timers for each LED blinking interval (e.g., 300ms)
unsigned long redPreviousMillis = 0;
unsigned long yellowPreviousMillis = 0;
unsigned long greenPreviousMillis = 0;
unsigned long bluePreviousMillis = 0;
unsigned long whitePreviousMillis = 0;

// State tracking: 0 = All OFF, 1 = Red, 2 = Yellow, 3 = Green 4 = Blue 5 = White(red)
int activeState = 0; 

// Debounce variables
bool lastRawState = HIGH;
bool previousButtonState = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
constexpr unsigned long debounceDelay = 50;

unsigned long pressStartTime = 0;

constexpr unsigned long LONG_PRESS_MS = 2000;
bool longPressHandled = false;

int blinkLedState = 0;
unsigned long blinkPreviousMillis = 0;
unsigned long currentBlinkInterval = 300; 

constexpr int NUM_SAMPLES = 10;
int samples[NUM_SAMPLES];
int sampleIndex = 0;
long samplesSum = 0;

constexpr unsigned long BLINK_MIN_MS = 50;    // slow
constexpr unsigned long BLINK_MAX_MS = 1000;  // fast


void updateLed(int pin, unsigned long interval, unsigned long &previousMillis);
void handleButtonPress();
void updateButton();
void updateActiveLed();
void setActiveState(int newState);
void serialBlink();
int readPotentiometer();
unsigned long potToInterval(int potValue);
void drawScreen();

void setup()
{
    Serial.begin(115200);

    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);

    if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        SCREEN_ADDRESS
    ))
    {
        Serial.println("OLED initialization failed!");
        while (true) {}
    }

    drawScreen();

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_WHITE, OUTPUT);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    int initial = analogRead(POT_PIN);
    for (int i = 0; i < NUM_SAMPLES; i++) {
        samples[i] = initial;
        samplesSum += initial;
    }

    Serial.println("ESP32 Blinker setup");
}

void loop()
{
    unsigned long now = millis();

    if (now - previousUpdate >= UPDATE_INTERVAL)
    {
        previousUpdate = now;

        drawScreen();
    }

     int potValue = readPotentiometer();
    currentBlinkInterval = potToInterval(potValue);

    updateButton();            

    if (longPressHandled) {
        serialBlink();
    } else {
        updateActiveLed();
    }
}

void updateLed(int pin, unsigned long interval, unsigned long &previousMillis)
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        digitalWrite(pin, !digitalRead(pin));
    }
}

void updateButton() 
{
    unsigned long now = millis();
    bool reading = digitalRead(BUTTON_PIN);

    // 1. Detect a raw signal transition.
    if (reading != lastRawState)
    {
        lastRawState = reading;
        lastDebounceTime = now;
    }

    // 2. Accept a new state after it stays stable.
    if (now - lastDebounceTime >= debounceDelay)
    {
        if (reading != buttonState)
        {
            buttonState = reading;

            if (buttonState == LOW)
            {
                // Confirmed press: falling edge.
                pressStartTime = now;
                longPressHandled = false;

                Serial.println("BUTTON PRESSED");
            }
            else
            {
                // Confirmed release: rising edge.
                Serial.println("BUTTON RELEASED");

                // A release before long-press handling
                // is a short press.
                if (!longPressHandled)
                {
                    Serial.println("SHORT PRESS");
                    //longPressHandled = false;
                    handleButtonPress();
                }
            }
        }
    }

    // 3. Detect a long press once per hold.
    if (buttonState == LOW &&
        lastRawState == LOW &&
        !longPressHandled &&
        now - pressStartTime >= LONG_PRESS_MS)
    {
        longPressHandled = true;

        Serial.println("LONG PRESS");
        //serialBlink();
    }
}

void handleButtonPress()
{
    // Cycle through states: 0 -> 1 -> 2 -> 3 -> 5 -> 6 -> 0
    activeState = (activeState + 1) % 6;
    
    Serial.print("Active State switched to: ");
    Serial.println(activeState);
    setActiveState(activeState);
}

void updateActiveLed() {
    switch (activeState)
    {
        case 1:
            updateLed(
                LED_RED, currentBlinkInterval, redPreviousMillis
            );
            break;

        case 2:
            updateLed(
                LED_YELLOW, currentBlinkInterval, yellowPreviousMillis
            );
            break;

        case 3:
            updateLed(
                LED_GREEN, currentBlinkInterval, greenPreviousMillis
            );
            break;
        case 4:
            updateLed(
                LED_BLUE, currentBlinkInterval, bluePreviousMillis
            );
            break;
        case 5:
            updateLed(
                LED_WHITE, currentBlinkInterval, whitePreviousMillis
            );
            break;

        case 0:
        default:
            // All LEDs remain OFF.
            break;
    }
}

void setActiveState(int newState)
{
    activeState = newState;

    // Turn off all LEDs before changing state.
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_WHITE, LOW);


    // Reset timers so the next blink starts predictably.
    unsigned long now = millis();

    redPreviousMillis = now;
    yellowPreviousMillis = now;
    greenPreviousMillis = now;
    bluePreviousMillis = now;
    whitePreviousMillis = now;

    Serial.print("Active state: ");
    Serial.println(activeState);
}

void serialBlink() {
    
    unsigned long currentMillis = millis();
    
    if (currentMillis - blinkPreviousMillis >= currentBlinkInterval) {
        blinkPreviousMillis = currentMillis;
        
        // Cycle through the LEDs one by one
        switch (blinkLedState) {
            case 0:
                digitalWrite(LED_RED, HIGH);
                digitalWrite(LED_YELLOW, LOW);
                digitalWrite(LED_GREEN, LOW);
                digitalWrite(LED_BLUE, LOW);
                digitalWrite(LED_WHITE, LOW);
                blinkLedState = 1;
                break;
            case 1:
                digitalWrite(LED_RED, LOW);
                digitalWrite(LED_YELLOW, HIGH);
                digitalWrite(LED_GREEN, LOW);
                digitalWrite(LED_BLUE, LOW);
                digitalWrite(LED_WHITE, LOW);
                blinkLedState = 2;
                break;
            case 2:
                digitalWrite(LED_RED, LOW);
                digitalWrite(LED_YELLOW, LOW);
                digitalWrite(LED_GREEN, HIGH);
                digitalWrite(LED_BLUE, LOW);
                digitalWrite(LED_WHITE, LOW);
                blinkLedState = 3;
                break;
            case 3:
                digitalWrite(LED_RED, LOW);
                digitalWrite(LED_YELLOW, LOW);
                digitalWrite(LED_GREEN, LOW);
                digitalWrite(LED_BLUE, HIGH);
                digitalWrite(LED_WHITE, LOW);
                blinkLedState = 4;
                break;
            case 4:
                digitalWrite(LED_RED, LOW);
                digitalWrite(LED_YELLOW, LOW);
                digitalWrite(LED_GREEN, LOW);
                digitalWrite(LED_BLUE, LOW);
                digitalWrite(LED_WHITE, HIGH);
                blinkLedState = 0;
                break;
        }
    }
}

int readPotentiometer()
{
    int raw = analogRead(POT_PIN);

    samplesSum -= samples[sampleIndex];
    samples[sampleIndex] = raw;
    samplesSum += raw;
    sampleIndex = (sampleIndex + 1) % NUM_SAMPLES;

    return samplesSum / NUM_SAMPLES;
}

unsigned long potToInterval(int potValue)
{
    constexpr long IN_MIN  = 0;
    constexpr long IN_MAX  = 4095;
    constexpr long OUT_MIN = BLINK_MIN_MS;   // 50
    constexpr long OUT_MAX = BLINK_MAX_MS;   // 1000

    if (potValue < IN_MIN) potValue = IN_MIN;
    if (potValue > IN_MAX) potValue = IN_MAX;

    int64_t num = (int64_t)(potValue - IN_MIN) * (OUT_MAX - OUT_MIN);
    int64_t den = (int64_t)(IN_MAX - IN_MIN);
    return (unsigned long)(num / den) + OUT_MIN;
}

void drawScreen()
{
    unsigned long uptimeSeconds = millis() / 1000;

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("ESP32 SYSTEM");

    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 20);
    display.println("Uptime:");

    display.setTextSize(2);
    display.setCursor(0, 35);
    display.print(uptimeSeconds);
    display.println(" sec");

    display.display();
}