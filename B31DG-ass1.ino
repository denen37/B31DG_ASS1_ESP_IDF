#define DEBUG_BUILD   // comment out for PRODUCTION

#ifdef DEBUG_BUILD
  #define TIME_SCALE 1000UL
#else
  #define TIME_SCALE 1UL
#endif

#define ON(pin) digitalWrite(pin, HIGH)
#define OFF(pin) digitalWrite(pin, LOW)


//Input pins
const uint8_t OUTPUT_ENABLE = 32;
const uint8_t OUTPUT_SELECT = 33;

//Output pins
const uint8_t SIGNAL_A = 16;
const uint8_t SIGNAL_B = 21;

//Delay parameters for interrupt
const unsigned long debounceDelay = 500;
volatile unsigned long lastPressTime_enable = 0;
volatile unsigned long lastPressTime_select = 0;

//Application state
volatile bool ENABLE = 1;
volatile bool NORMAL = 1;


//timing parameters
const unsigned long T_ON1     = 100UL;   // 100 µs base
const unsigned long T_OFF     = 400UL * TIME_SCALE;   // 400 µs
const unsigned int  NUM       = 5;                    // number of pulses
const unsigned long T_WAIT    = 4500UL * TIME_SCALE;  // 4500 µs
const unsigned long T_SYNC_ON = 50UL * TIME_SCALE;    // 50 µs

unsigned int getT_ON(unsigned int n){
  return (n > 1 ? T_ON1 + (n - 1) * 50  : T_ON1) * TIME_SCALE;
}

void toggleSwitch() {
  if(afterDebounceDelay(lastPressTime_enable)){
    ENABLE = !ENABLE;
    Serial.println("OUTPUT_ENABLE has been pressed!");
  }

  lastPressTime_enable = millis();
}

void toggleWaveform() {
  if(afterDebounceDelay(lastPressTime_select)){
    NORMAL = !NORMAL;
    Serial.println("OUTPUT_SELECT has been pressed!");
  }

  lastPressTime_select = millis();
}

bool afterDebounceDelay(unsigned long lastPressTime){
  unsigned long now = millis();
  
  return now - lastPressTime > debounceDelay;;
}


void setup() {
  Serial.begin(9600);

  pinMode(OUTPUT_ENABLE, INPUT);
  pinMode(OUTPUT_SELECT, INPUT);

  pinMode(SIGNAL_A, OUTPUT);
  pinMode(SIGNAL_B, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(OUTPUT_ENABLE), toggleSwitch, RISING);
  attachInterrupt(digitalPinToInterrupt(OUTPUT_SELECT), toggleWaveform, RISING);
}

void loop() {

  if(ENABLE){
    if(NORMAL){
      //normal form
      OFF(SIGNAL_A);
      ON(SIGNAL_B);
      delayMicroseconds(T_SYNC_ON);

      for(int i = 1; i <= NUM; i++){
        ON(SIGNAL_A);
        OFF(SIGNAL_B);
        delayMicroseconds(getT_ON(i));

        OFF(SIGNAL_A);
        delayMicroseconds(T_OFF);
      }
    }else{
      // alternative form
      OFF(SIGNAL_A);
      ON(SIGNAL_B);
      delayMicroseconds(T_SYNC_ON);

      for(int i = NUM; i >= 1; i--){
        ON(SIGNAL_A);
        OFF(SIGNAL_B);
        delayMicroseconds(getT_ON(i));

        OFF(SIGNAL_A);
        delayMicroseconds(T_OFF);
      }
    }
  }
}

