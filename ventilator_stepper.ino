

// library include
#include "configuration.h"
#include <Stepper.h>

#ifdef LCD_TYPE_PARALLEL
#include <LiquidCrystal.h>

LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);
#endif

#ifdef LCD_TYPE_I2C
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

#endif

// object and variable declaration
Stepper stepper(MOTOR_STEPS, MOTOR_A_PIN, MOTOR_B_PIN, MOTOR_C_PIN, MOTOR_D_PIN);
long motor_position;
long target_position;

int bpm; // breath per minute
int volume_step; //air volume to pump
unsigned long next_state_time;
unsigned long next_ui_time;

enum  {
  stINHALE,
  stEXHALE
} state;

void ui_control(void) {
  if (millis()>=next_ui_time) {
    int bpm_temp = analogRead(BPM_PIN);
    int volume_temp = analogRead(VOLUME_PIN);
  
    // calculate BPM by converting the range of the ADC
    bpm = MIN_BPM + bpm_temp*(BPM_STEPS-1)/MAX_ADC_VALUE*BPM_INCREMENT;
    
    volume_step = volume_temp*VOLUME_STEPS/MAX_ADC_VALUE;
  
    lcd.setCursor(0, 1);
    lcd.print("RATE:   "); // clear previous value
    lcd.setCursor(5, 1);
    lcd.print(bpm);       // and print new one...
    lcd.setCursor(8, 1);
    lcd.print("VOL:   ");
    lcd.setCursor(12, 1);
    lcd.print(MIN_VOLUME+(volume_step-1)*VOLUME_INCREMENT);
    next_ui_time = millis()+UI_REFRESH_PERIOD;
  }
}

// here we check if it's time to change from inhalation to exhalation
// whenever we change state we set up deadlines for next state and
// in case there's no position feedback, for when the motor should stop
void state_advance(void) {
  if (millis()>=next_state_time) {
    switch (state) {
      case stINHALE : 
//        lcd.setCursor(0, 0);
//        lcd.print("EXHALE");
        state = stEXHALE;
        target_position = motor_positions[0];
        next_state_time=millis()+EXHALE_DURATION;
        break;
      case stEXHALE : 
//        lcd.setCursor(0, 0);
//        lcd.print("INHALE");
        state = stINHALE;
        target_position = motor_positions[volume_step];
        next_state_time=millis()+INHALE_DURATION;
        break;
    }
  }
}

void motor_control_loop(void) {
  long delta = target_position - motor_position;
  if (abs(delta)>0) {
    unsigned long steps;
    if (delta > MOTOR_MAX_STEPS_PER_ITERATION) steps = MOTOR_MAX_STEPS_PER_ITERATION;
    else if (delta < -MOTOR_MAX_STEPS_PER_ITERATION) steps = -MOTOR_MAX_STEPS_PER_ITERATION;
    else steps = delta;
    stepper.step(steps);
    motor_position +=steps;
  } 
}

void setup() {
#ifdef LCD_TYPE_I2C
  lcd.init();                      // initialize the lcd 
  // Print a message to the LCD.
  lcd.backlight();
#endif
#ifdef LCD_TYPE_PARALLEL
  lcd.begin(16, 2);
#endif
  // set up motor speed
  stepper.setSpeed(MOTOR_SPEED_RPM);
  
  //initialize motor
  lcd.setCursor(0, 0);
  lcd.print("VENTILATOR ON");
#ifdef AUTO_ZERO
  while (digitalRead(ZERO_PIN)) {
    stepper.step(-1);
  }
  // now that motor is at zero we can reset its position
#endif
  delay(1000);
  motor_position = 0;
  lcd.setCursor(0, 0);
  lcd.print("             ");
  
  // default values for volume and bpm
  volume_step =3;
  bpm = 16;
  
  target_position = motor_positions[volume_step];
//  lcd.setCursor(0, 0);
//  lcd.print("INHALE");
  state = stINHALE;
  next_state_time = millis()+INHALE_DURATION;
  next_ui_time = millis()+UI_REFRESH_PERIOD;
  

}

void loop() {
  motor_control_loop();
  state_advance();
  ui_control();
}
