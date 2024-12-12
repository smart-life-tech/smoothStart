#include <Arduino.h>
#include <Ticker.h>

Ticker pwmTicker;

const int DIR = 23;    // Direction pin
const int STEP = 5;    // Step pin
const int EN_PIN = 21; // Enable pin

const int steps_per_rev = 200; // Steps per revolution
int targetSpeed = 60;          // Target speed in RPM
int currentSpeed = 0;          // Current speed in RPM
const int accelRate = 5;       // Acceleration step (RPM per increment)
const int accelDelay = 1000;     // Delay between acceleration steps (ms)

int freq = 0; // Frequency for step pulses
void stepMotor();
int RevToFreq(int);

void setup()
{
    Serial.begin(115200);

    pinMode(STEP, OUTPUT);
    pinMode(DIR, OUTPUT);
    pinMode(EN_PIN, OUTPUT);

    digitalWrite(EN_PIN, LOW); // Enable motor

    // Set initial rotation direction
    digitalWrite(DIR, LOW); // LOW for clockwise, HIGH for counterclockwise

    // Attach ticker for step pulses
    pwmTicker.attach_ms(1000 / (freq > 0 ? freq : 1), stepMotor);
}

void loop()
{
    // Smoothly ramp up or down speed
    if (currentSpeed < targetSpeed)
    {
        currentSpeed += accelRate;
        if (currentSpeed > targetSpeed)
            currentSpeed = targetSpeed;
    }
    else if (currentSpeed > targetSpeed)
    {
        currentSpeed -= accelRate;
        if (currentSpeed < targetSpeed)
            currentSpeed = targetSpeed;
    }

    // Update frequency based on current speed
    freq = RevToFreq(currentSpeed);
    pwmTicker.attach_ms(1000 / (freq > 0 ? freq : 1), stepMotor);

    // Debugging output
    Serial.print("Target Speed: ");
    Serial.print(targetSpeed);
    Serial.print(" RPM | Current Speed: ");
    Serial.println(currentSpeed);

    delay(accelDelay); // Adjust update interval for smoother ramping
}

void stepMotor()
{
    static bool state = false;
    state = !state;
    digitalWrite(STEP, state ? HIGH : LOW);
}

int RevToFreq(int Rev)
{
    float tempFrequency = (200.0 / 60.0) * Rev; // Convert RPM to frequency
    return (int)round(tempFrequency);
}
