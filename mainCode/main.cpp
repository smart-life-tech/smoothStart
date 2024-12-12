#include <Ticker.h>
#include <Arduino.h>

Ticker pwmTicker;

const int DIR = 23;
const int STEP = 5;
const int SOLENOID = 15;
const int INTERUPT_PIN = 17;
const int EN_PIN = 21;

#define CH2 25
#define CH3 33

int RevToFreq(int Rev);
void rotateClockWise(void);
void rotateAntiClockWise(void);
void turnOFFsolenoid();
void turnONsolenoid();
void motorStopped(void);
void interruptRoutine(void);
void updateMotorSpeed();

int ch2Value;
int ch3Value;
int16_t position = 0;
volatile bool interrupt = false;
volatile bool rotating_clockwise = false;
volatile bool rotating_anticlockwise = false;

const int steps_per_rev = 200;
int target_speed = 0;               // Desired speed (RPM)
int current_speed = 0;              // Current speed (RPM)
const int acceleration_step = 1;    // RPM increment per step
const int acceleration_delay = 500; // Delay between speed changes (ms)

const int PWM_CHANNEL = 0;
int freq = 300;
const int resolution = 8;
const int MAX_DUTY_CYCLE = (int)(pow(2, resolution) - 1);
// If the channel is off, return the default value
int readChannel(int channelInput, int minLimit, int maxLimit, int defaultValue)
{
    int ch = pulseIn(channelInput, HIGH, 30000);
    if (ch < 100)
        return defaultValue;
    return map(ch, 1000, 2000, minLimit, maxLimit);
}
void IRAM_ATTR interruptRoutine()
{
    if (rotating_anticlockwise)
    {
        position -= 1;
        if (position == -1)
        {
            position = 200;
        }
    }
    else if (rotating_clockwise)
    {
        position += 1;
        if (position == 201)
        {
            position = 0;
        }
    }
    if (position == 190)
    {
        turnONsolenoid();
    }
    if (position == 100)
    {
        turnOFFsolenoid();
    }
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

    pwmTicker.attach_ms(1000 / freq, []()
                        {
        static bool state = false;
        state = !state;
        digitalWrite(STEP, state ? HIGH : LOW); });

    pinMode(CH2, INPUT);
    pinMode(CH3, INPUT);
}

void loop()
{
    ch2Value = readChannel(CH2, -100, 100, 0);
    ch3Value = readChannel(CH3, -100, 100, 0);

    if (ch3Value < -110)
    {
        digitalWrite(EN_PIN, HIGH);
        turnOFFsolenoid();
        motorStopped();
        target_speed = 0;
    }
    else if (ch3Value > 8)
    {
        rotateClockWise();
        digitalWrite(EN_PIN, LOW);
        target_speed = map(ch3Value, 9, 100, 30, 180);
    }
    else if (ch3Value < -8)
    {
        rotateAntiClockWise();
        digitalWrite(EN_PIN, LOW);
        target_speed = map(ch3Value, -9, -100, 30, 180);
    }
    else
    {
        digitalWrite(EN_PIN, HIGH);
        turnOFFsolenoid();
        motorStopped();
        target_speed = 0;
    }

    if (current_speed != target_speed)
    {
        if (current_speed < target_speed)
        {
            current_speed += acceleration_step;
            if (current_speed > target_speed)
            {
                current_speed = target_speed;
            }
        }
        else if (current_speed > target_speed)
        {
            current_speed -= acceleration_step;
            if (current_speed < target_speed)
            {
                current_speed = target_speed;
            }
        }
        freq = RevToFreq(current_speed);
        pwmTicker.attach_ms(1000 / freq, []()
                            {
            static bool state = false;
            state = !state;
            digitalWrite(STEP, state ? HIGH : LOW); });
        delay(acceleration_delay);
    }

    Serial.print(" | Ch2: ");
    Serial.print(ch2Value);
    Serial.print(" | Ch3: ");
    Serial.print(ch3Value);
    Serial.print(" | Target Speed: ");
    Serial.print(target_speed);
    Serial.print(" | Current Speed: ");
    Serial.println(current_speed);
}

int RevToFreq(int Rev)
{
    float tempFrequency = (200.0 / 60.0) * Rev;
    int frequency = (int)round(tempFrequency);
    return frequency;
}

void rotateClockWise(void)
{
    rotating_clockwise = true;
    rotating_anticlockwise = false;
    digitalWrite(DIR, LOW);
}

void rotateAntiClockWise(void)
{
    rotating_anticlockwise = true;
    rotating_clockwise = false;
    digitalWrite(DIR, HIGH);
}

void motorStopped(void)
{
    rotating_anticlockwise = false;
    rotating_clockwise = false;
}

void turnONsolenoid()
{
    digitalWrite(SOLENOID, HIGH);
}

void turnOFFsolenoid()
{
    digitalWrite(SOLENOID, LOW);
}
