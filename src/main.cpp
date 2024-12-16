
#include <Arduino.h>
#include <Ticker.h>

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

// Integers to represent values from sticks and pots
int ch2Value;
int ch3Value;


int16_t position = 0;
volatile bool interrupt = false;
volatile bool interrupt_clockwise = false;
volatile bool interrupt_anticlockwise = false;

volatile bool rotating_clockwise = false;
volatile bool rotating_anticlockwise = false;



const int  steps_per_rev = 200;

int speed = 100;  // speed in Rev / Min

const int PWM_CHANNEL = 0;    // ESP32 has 16 channels which can generate 16 independent waveforms
int freq = 300;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int resolution = 8; // We'll use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits 

// The max duty cycle value based on PWM resolution (will be 255 if resolution is 8 bits)
const int MAX_DUTY_CYCLE = (int)(pow(2, resolution) - 1); 


// Read the number of a specified channel and convert to the range provided.
// If the channel is off, return the default value
int readChannel(int channelInput, int minLimit, int maxLimit, int defaultValue){
  int ch = pulseIn(channelInput, HIGH, 30000);
  if (ch < 100) return defaultValue;
  return map(ch, 1000, 2000, minLimit, maxLimit);
}
 
// Read the switch channel and return a boolean value
bool readSwitch(byte channelInput, bool defaultValue){
  int intDefaultValue = (defaultValue)? 100: 0;
  int ch = readChannel(channelInput, 0, 100, intDefaultValue);
  return (ch > 50);
}

void IRAM_ATTR interruptRoutine()
{
   if( rotating_anticlockwise )
   {
    position -= 1;
    if( position == -1 )
    {
      position = 200;
    }
   }

   else if( rotating_clockwise )
   {
    position += 1;
    if( position == 201 )
    {
      position = 0;
    }
   }

  if( position == 10                                                                                                                                             )
  {
    turnONsolenoid();
  }

  if( position == 100 )
  {
    turnOFFsolenoid();
  }

   //Serial.println(position);
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


  freq = RevToFreq(speed);
    // Sets up a channel (0-15), a PWM duty cycle frequency, and a PWM resolution (1 - 16 bits) 
  // ledcSetup(uint8_t channel, double freq, uint8_t resolution_bits);
  // Initialize the Servo on the specified pin

  pwmTicker.attach_ms(1000 / freq, []() {
    static bool state = false;
    state = !state;
    digitalWrite(STEP, state ? HIGH : LOW);
  });
    // Set all pins as inputs
  pinMode(CH2, INPUT);
  pinMode(CH3, INPUT);
}

void loop()
{
    // Get values for each channel
  ch2Value = readChannel(CH2, -100, 100, 0);
  ch3Value = readChannel(CH3, -100, 100, -100);

  if( ch3Value < -110 )
  {
    digitalWrite(EN_PIN, HIGH);
    turnOFFsolenoid();
    motorStopped();
  }

  else if( ch2Value > 4 )
  {
    rotateClockWise();
    digitalWrite(EN_PIN, LOW);
    speed = map(ch2Value, 5, 50, 30, 180);
  }

  else if( ch2Value < -4 )
  {
    rotateAntiClockWise();
    digitalWrite(EN_PIN, LOW);
    speed = map(ch2Value, -5, -50, 30, 180);
  }

  else{
    digitalWrite(EN_PIN, HIGH);
    turnOFFsolenoid();
    motorStopped();
  }

  freq = RevToFreq(speed);

    pwmTicker.attach_ms(1000 / freq, []() {
    static bool state = false;
    state = !state;
    digitalWrite(STEP, state ? HIGH : LOW);
  });
  
  // //Print to Serial Monitor
  // Serial.print(" | Ch2: ");
  // Serial.print(ch2Value);
  // Serial.print(" | Ch3: ");
  // Serial.println(ch3Value);

  //Serial.println(position);

}

int RevToFreq( int Rev )
{
    float tempFrequency = (200.0 / 60.0) * Rev;
    int frequency = (int)round(tempFrequency);
    //Serial.println(frequency);
    return frequency;
}

void rotateClockWise(void)
{
  rotating_clockwise = true;
  digitalWrite(DIR, LOW);
}

void rotateAntiClockWise(void)
{
  rotating_anticlockwise = true;
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