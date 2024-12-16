#include <Arduino.h>
#include <Ticker.h>
#include <math.h>

// Constants
const int DIR = 23;
const int STEP = 5;
const int SOLENOID = 15;
const int INTERUPT_PIN = 17;
const int EN_PIN = 21;

#define CH2 25
#define CH3 33

Ticker pwmTicker;

// Function declarations
int RevToFreq(int Rev);
void rotateClockWise(void);
void rotateAntiClockWise(void);
void turnOFFsolenoid();
void turnONsolenoid();
void motorStopped(void);
void interruptRoutine(void);
void updateMotorSpeed();
void handleSerialCommands();
int readChannel(int, int, int, int);
int calculateSpeed(int);
// Global variables
int ch2Value;
int ch3Value;
int16_t position = 0;

volatile bool interrupt_clockwise = false;
volatile bool interrupt_anticlockwise = false;
volatile bool rotating_clockwise = false;
volatile bool rotating_anticlockwise = false;

const int steps_per_rev = 200;

int speed = 100; // speed in Rev / Min
int freq = 300;  // Initial frequency
const int PWM_CHANNEL = 0;
const int resolution = 8; // 8-bit resolution for PWM
const int MAX_DUTY_CYCLE = (int)(pow(2, resolution) - 1);

// Dynamic speed limits
int minSpeed = 30;  // Minimum speed in RPM
int maxSpeed = 180; // Maximum speed in RPM

// Interrupt service routine
void IRAM_ATTR interruptRoutine()
{
    if (rotating_anticlockwise)
    {
        position -= 1;
        if (position == -1)
            position = 200;
    }
    else if (rotating_clockwise)
    {
        position += 1;
        if (position == 201)
            position = 0;
    }

    if (position == 10)
        turnONsolenoid();
    if (position == 100)
        turnOFFsolenoid();
}

void setup()
{
    Serial.begin(115200);

    pinMode(STEP, OUTPUT);
    pinMode(DIR, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(SOLENOID, OUTPUT);
    pinMode(INTERUPT_PIN, INPUT);

    digitalWrite(EN_PIN, HIGH);
    attachInterrupt(INTERUPT_PIN, interruptRoutine, RISING);

    // Set initial PWM frequency
    freq = RevToFreq(speed);
    pwmTicker.attach_ms(1000 / freq, []()
                        {
    static bool state = false;
    state = !state;
    digitalWrite(STEP, state ? HIGH : LOW); });

    // Set all pins as inputs
    pinMode(CH2, INPUT);
    pinMode(CH3, INPUT);
}

void loop()
{
    // Read values from the channels
    ch2Value = readChannel(CH2, -100, 100, 0);
    ch3Value = readChannel(CH3, -100, 100, -100);

    if (ch3Value < -110)
    { // Stop motor
        digitalWrite(EN_PIN, HIGH);
        turnOFFsolenoid();
        motorStopped();
    }
    else if (ch2Value > 4)
    { // Rotate clockwise
        rotateClockWise();
        digitalWrite(EN_PIN, LOW);
        speed = calculateSpeed(ch2Value); // Use non-linear speed mapping
    }
    else if (ch2Value < -4)
    { // Rotate anticlockwise
        rotateAntiClockWise();
        digitalWrite(EN_PIN, LOW);
        speed = calculateSpeed(ch2Value); // Use non-linear speed mapping
    }
    else
    { // Stop motor
        digitalWrite(EN_PIN, HIGH);
        turnOFFsolenoid();
        motorStopped();
    }

    // Update motor frequency
    static int lastFreq = 0;
    freq = RevToFreq(speed);
    if (freq != lastFreq)
    {
        pwmTicker.attach_ms(1000 / freq, []()
                            {
      static bool state = false;
      state = !state;
      digitalWrite(STEP, state ? HIGH : LOW); });
        lastFreq = freq;
    }

    // Handle serial commands for dynamic speed adjustment
    handleSerialCommands();

    // Debugging information
    Serial.print("Speed: ");
    Serial.print(speed);
    Serial.print(" | Frequency: ");
    Serial.println(freq);
}

// Calculate frequency based on speed (RPM)
int RevToFreq(int Rev)
{
    float tempFrequency = (200.0 / 60.0) * Rev; // steps/rev * (revolutions/sec)
    return (int)round(tempFrequency);
}

// Rotate clockwise
void rotateClockWise(void)
{
    rotating_clockwise = true;
    digitalWrite(DIR, LOW);
}

// Rotate anticlockwise
void rotateAntiClockWise(void)
{
    rotating_anticlockwise = true;
    digitalWrite(DIR, HIGH);
}

// Stop motor
void motorStopped(void)
{
    rotating_anticlockwise = false;
    rotating_clockwise = false;
}

// Turn solenoid ON
void turnONsolenoid()
{
    digitalWrite(SOLENOID, HIGH);
}

// Turn solenoid OFF
void turnOFFsolenoid()
{
    digitalWrite(SOLENOID, LOW);
}

// Read channel input and convert to range
int readChannel(int channelInput, int minLimit, int maxLimit, int defaultValue)
{
    int ch = pulseIn(channelInput, HIGH, 30000);
    if (ch < 100)
        return defaultValue;
    return map(ch, 1000, 2000, minLimit, maxLimit);
}

// Custom speed calculation with non-linear mapping
int calculateSpeed(int inputValue)
{
    inputValue = abs(inputValue); // Ensure positive value
    return minSpeed + pow((float)inputValue / 100, 2) * (maxSpeed - minSpeed);
}

// Handle serial commands for dynamic speed adjustment
void handleSerialCommands()
{
    if (Serial.available())
    {
        String command = Serial.readStringUntil('\n');
        if (command.startsWith("SET_MAX"))
        {
            maxSpeed = command.substring(8).toInt();
            Serial.print("Max speed set to: ");
            Serial.println(maxSpeed);
        }
        else if (command.startsWith("SET_MIN"))
        {
            minSpeed = command.substring(8).toInt();
            Serial.print("Min speed set to: ");
            Serial.println(minSpeed);
        }
    }
}
