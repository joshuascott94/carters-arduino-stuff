// Include Wire for...
#include <Wire.h>

// Include LCD display driver and configure I2C
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <BH1750.h>
BH1750 lightMeter;

// include the library code:
// define the DHT11 as the digital pin 7
#include <dht11.h>      
dht11 DHT;
#define DHT11_PIN 7     

// define the name of the servo rotating right and left
// define the name of the servo rotating upwards and downwards
#include <Servo.h>
//#include <SlowMotionServo.h>
//Servo lr_servo;         
//Servo ud_servo;
//unsigned long MOVING_TIME = 3000; // moving time is 3 seconds
//unsigned long moveStartTime;

 // set button to pin 2;
bool button_pressed = false;
const byte buttonPin = 2;   
byte lastButtonState = LOW;
unsigned long debounceDuration = 50; // millis
unsigned long lastTimeButtonStateChanged = 0;        

// set the initial angle to 90 degree
// set the initial angle to 10 degree; keep the solar panels upright to detect the strongest light
// define the analog voltage input of the photoresistors
int lr_angle = 90;                  
int ud_angle = 10;  
int l_state = A0;   
int r_state = A1;
int u_state = A2;
int d_state = A3;

//set the pin of the buzzer to digital pin 6
const byte buzzer = 6;

//define the control signal pin of the servo rotating right and lef
const byte lr_servopin = 9;   

//define the control signal pin of the servo rotating clockwise and anticlockwise
const byte ud_servopin = 10;  

unsigned int light;   //save the variable of light intensity
byte error = 10;      //Define the error range to prevent vibration
byte m_speed = 1000;    //set delay time to adjust the speed of servo;the longer the time, the smaller the speed
byte resolution = 1;  //set the rotation accuracy of the servo, the minimum rotation angle
int temperature;      //save the variable of temperature
int humidity;         //save the variable of humidity

// for buzzer stuff
unsigned long previousMillis = 0;
unsigned long pauseBetweenNotes;
int thisNote;

// make your own servo class
class SlowServo {
protected:
  uint16_t target = 90;   // target angle
  uint16_t current = 90;  // current angle
  uint8_t interval = 50;  // delay time
  uint32_t previousMillis = 0;
public:
  Servo servo;

  void begin(byte pin) {
    servo.attach(pin);
  }

  void setSpeed(uint8_t newSpeed) {
    interval = newSpeed;
  }

  void set(uint16_t newTarget) {
    target = newTarget;
  }

  void update() {
    if (millis() - previousMillis > interval) {
      previousMillis = millis();
      if (target < current) {
        current--;
        servo.write(current);
      } else if (target > current) {
        current++;
        servo.write(current);
      }
    }
  }
};

SlowServo lr_servo;
SlowServo ud_servo;

void setup() {

  //define the serial baud rate
  Serial.begin(9600);  

  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin();
  lightMeter.begin();

  //Servo lr_servo;         
  //Servo ud_servo;


  // set the control pin of left-right servo
  //lr_servo.attach(lr_servopin);
  lr_servo.begin(lr_servopin);

  // set the control pin of up-down servo
  //ud_servo.attach(ud_servopin);  
  ud_servo.begin(ud_servopin);

  // set the mode of servo pins
  pinMode(l_state, INPUT);       
  pinMode(r_state, INPUT);
  pinMode(u_state, INPUT);
  pinMode(d_state, INPUT);

  // set the button pin is set to input pull-up mode
  // and attach external interrupt touch type is falling edge; adjust_resolution is interrupt service function ISR    
  pinMode(buttonPin, INPUT_PULLUP);         
  //attachInterrupt(digitalPinToInterrupt(buttonPin), play_song, FALLING);  

   // initialize the LCD and set LCD backlight
  lcd.init();      
  lcd.backlight();  

  //return to initial angle
  //lr_servo.write(lr_angle);
  //delay(1000);
  //ud_servo.write(ud_angle);
  //delay(1000);
  lr_servo.set(lr_angle);
  ud_servo.set(ud_angle);

  lr_servo.update();
  ud_servo.update();


  
}

void loop() {
  //servo performs the action
  ServoAction();

  //read the light intensity of bh1750
  read_light();

  //read the value of temperature and humidity
  read_dht11();

  //Lcd shows the values of light intensity, temperature and humidity
  LcdShowValue();  

  byte buttonState = digitalRead(buttonPin);
  if (millis() - lastTimeButtonStateChanged > debounceDuration) {
      byte buttonState = digitalRead(buttonPin);
      if (buttonState != lastButtonState) {
      lastTimeButtonStateChanged = millis();
      lastButtonState = buttonState;
      if (buttonState == LOW) {
        // do an action, for example print on Serial
        play_song();
        //Serial.println("Button Pressed");
      }
    }
  }

  
}

/**********the function of the servo************/
void ServoAction() {
  //read the analog voltage value of the sensor, 0-1023
  int L = analogRead(l_state);  
  int R = analogRead(r_state);
  int U = analogRead(u_state);
  int D = analogRead(d_state);

  /**********************system adjusting left and right序**********************/
  //  abs() is the absolute value function
  if (abs(L - R) > error && L > R)  {                         
    
    // Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    lr_angle -= resolution; 
    
     // limit the rotation angle of the servo
    if (lr_angle < 0) { 
      lr_angle = 0;
    }

    // output the angle of the servo
    //lr_servo.write(lr_angle); 
    //delay(m_speed);
    lr_servo.set(lr_angle);

  } else if (abs(L - R) > error && L < R)  {                         
    
    // Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    lr_angle += resolution; 
    
    // limit the rotation angle of servo
    if (lr_angle > 180) { 
      lr_angle = 180;
    }

    // output the angle of servo
    // lr_servo.write(lr_angle); 
    //delay(m_speed);
    lr_servo.set(lr_angle);
  
  } else if (abs(L - R) <= error) { 
    
    // Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    //lr_servo.write(lr_angle); 
    lr_servo.set(lr_angle);
  }
  
  // # # # # # # # # # # # # # # # # #
  // Adjust the up/down servo
  // # # # # # # # # # # # # # # # # #
  if (abs(U - D) > error && U >= D) {
    
    // Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    ud_angle -= resolution; 
    
    // limit the rotation angle of servo
    if (ud_angle < 10) { 
      ud_angle = 10;
    }

    // output the angle of servo
    // ud_servo.write(ud_angle); 
    // delay(m_speed);
    ud_servo.set(ud_angle);
  
  } else if (abs(U - D) > error && U < D)  {
    
    // Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    ud_angle += resolution;

    // limit the rotation angle of servo
    if (ud_angle > 90) { 
      ud_angle = 90;
    }

    // output the angle of servo
    // ud_servo.write(ud_angle); 
    // delay(m_speed);
    ud_servo.set(ud_angle);

  } else if (abs(U - D) <= error) { 
    
    // Determine whether the error is within the acceptable range. If it is, keep it stable and make no change in angle
    // ud_servo.write(ud_angle);
    ud_servo.set(ud_angle);
  }

  // //Serial monitor displays the resistance of the photoresistor and the angle of servo
  // Serial.print(" L ");
  // Serial.print(L);
  // Serial.print(" R ");
  // Serial.print(R);
  // Serial.print("  U ");
  // Serial.print(U);
  // Serial.print(" D ");
  // Serial.print(D);
  // Serial.print("  ud_angle ");
  // Serial.print(ud_angle);
  // Serial.print("  lr_angle ");
  // Serial.println(lr_angle);
  // delay(1000);  //During the test, the serial port data is received too fast, and it can be adjusted by adding delay time 

  // Update slow servo
  lr_servo.update();
  ud_servo.update();
  
  
}

void LcdShowValue() {
  char str1[5];
  char str2[2];
  char str3[2];

  // Format the light value data as a string, left-aligned
  dtostrf(light, -5, 0, str1);  
  dtostrf(temperature, -2, 0, str2);
  dtostrf(humidity, -2, 0, str3);

  // LCD1602 display
  // display the value of the light intensity
  lcd.setCursor(0, 0);
  lcd.print("Light:");
  lcd.setCursor(6, 0);
  lcd.print(str1);
  lcd.setCursor(11, 0);
  lcd.print("lux");

  // display the value of temperature and humidity
  lcd.setCursor(0, 1);
  lcd.print((int)round(1.8 * temperature + 32));
  lcd.setCursor(2, 1);
  lcd.print("F");
  lcd.setCursor(5, 1);
  lcd.print(humidity);
  lcd.setCursor(7, 1);
  lcd.print("%");

  //show the accuracy of rotation
  // lcd.setCursor(11, 1);
  // lcd.print("res:");
  // lcd.setCursor(15, 1);
  // lcd.print(resolution);
  /*if (light < 10) {
    lcd.setCursor(7, 0);
    lcd.print("        ");
    lcd.setCursor(6, 0);
    lcd.print(light);
    } else if (light < 100) {
    lcd.setCursor(8, 0);
    lcd.print("       ");
    lcd.setCursor(6, 0);
    lcd.print(light);
    } else if (light < 1000) {
    lcd.setCursor(9, 0);
    lcd.print("      ");
    lcd.setCursor(6, 0);
    lcd.print(light);
    } else if (light < 10000) {
    lcd.setCursor(9, 0);
    lcd.print("      ");
    lcd.setCursor(6, 0);
    lcd.print(light);
    } else if (light < 100000) {
    lcd.setCursor(10, 0);
    lcd.print("     ");
    lcd.setCursor(6, 0);
    lcd.print(light);
    }*/
}

void read_light() {
  //read the light intensity detected by BH1750
  light = lightMeter.readLightLevel();  
}

void read_dht11() {
  int chk;
  // read data
  chk = DHT.read(DHT11_PIN);  
  switch (chk) {
    case DHTLIB_OK:
      break;
    //check and return error
    case DHTLIB_ERROR_CHECKSUM:
      break;
     //Timeout and return error
    case DHTLIB_ERROR_TIMEOUT: 
      break;
    default:
      break;
  }
  temperature = DHT.temperature;
  humidity = DHT.humidity;
}

/********* function disrupts service **************/
void button_function() {

  // delay(10);  //delay to eliminate vibration
  if (!digitalRead(buttonPin)) {
    button_pressed = true;
  }
}

void play_song() {
  #define NOTE_C3 131
  #define NOTE_CS3 139
  #define NOTE_D3 147
  #define NOTE_DS3 156
  #define NOTE_E3 165
  #define NOTE_F3 175
  #define NOTE_FS3 185
  #define NOTE_G3 196
  #define NOTE_GS3 208
  #define NOTE_A3 220
  #define NOTE_AS3 233
  #define NOTE_Bb3 241
  #define NOTE_B3 247
  #define NOTE_C4 262
  #define NOTE_CS4 277
  #define NOTE_D4 294
  #define NOTE_DS4 311
  #define NOTE_E4 330
  #define NOTE_F4 349
  #define NOTE_FS4 370
  #define NOTE_G4 392
  #define NOTE_GS4 415
  #define NOTE_A4 440
  #define NOTE_AS4 466
  #define NOTE_Bb4 482
  #define NOTE_B4 494
  #define NOTE_C5 523
  #define NOTE_CS5 554
  #define NOTE_D5 587
  #define NOTE_DS5 622
  #define NOTE_E5 659
  #define NOTE_F5 698
  #define NOTE_FS5 740
  #define NOTE_G5 784
  #define NOTE_GS5 831
  #define NOTE_A5 880
  #define NOTE_AS5 932
  #define NOTE_B5 988
  #define REST 0

  int melody[] = {
    // 

    // Notes go here
    NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_Bb3, 8, NOTE_A3, 4, REST, 8, 
    NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_Bb3, 8, NOTE_A3, 4,
    NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 8, NOTE_C4,  4, NOTE_Bb3,4, 
    NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_C4,  4, NOTE_Bb3,4,

    NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_Bb3, 8, NOTE_A3, 4, REST, 8,
    NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_Bb3, 8, NOTE_A3, 4, 
    NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 8, NOTE_C4, 4, NOTE_Bb3, 4, 
    NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_C4, 4, NOTE_Bb3, 4,

    NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 8, NOTE_D4, 4, NOTE_F4, 2,
    NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_F4, 2,
    NOTE_Bb3,4, NOTE_Bb3,4, NOTE_Bb3,4, NOTE_Bb3,4, NOTE_Bb3,4, NOTE_Bb3,4,
    NOTE_C4, 4, NOTE_Bb3,4, NOTE_A3, 2, NOTE_Bb3,4, NOTE_C4, 2, NOTE_F4, 8,

    NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4,
    NOTE_A3, 4, NOTE_G3, 4, NOTE_F3, 4, NOTE_G3, 2, NOTE_A3, 2, NOTE_A3, 4,
    NOTE_B3, 8, NOTE_B3, 4, NOTE_B3, 4, NOTE_B3, 4, NOTE_B3, 4, NOTE_B3, 4,
    NOTE_C4, 4, NOTE_D4, 4, NOTE_E4, 4, NOTE_D4, 4, NOTE_C4, 8, REST, 4,

    NOTE_C4, 2, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_Bb3, 4, NOTE_A3, 8,
    NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_Bb3,4, NOTE_A3, 8,
    NOTE_C4, 4, NOTE_D4, 4, NOTE_F4, 4, NOTE_F4, 4, NOTE_D4, 4,
    NOTE_F4, 4, NOTE_D4, 4, NOTE_F4, 4, NOTE_G4, 4, NOTE_A4, 4,
    REST, 2,

    NOTE_A4, 4, NOTE_G4, 4, NOTE_F4, 4, NOTE_D4, 4, 
    NOTE_F4, 4, NOTE_G4, 4, NOTE_F4, 4, NOTE_F4, 4,
    NOTE_G4, 4, NOTE_A4, 4, NOTE_C5, 4, NOTE_C5, 4,
    NOTE_A4, 4, NOTE_D5, 4, NOTE_C5, 4, NOTE_C5, 8,

    NOTE_A4, 4, NOTE_A4, 4, NOTE_A4, 4, NOTE_F4, 4, NOTE_F4, 4, 
    NOTE_A4, 4, NOTE_A4, 4, NOTE_F4, 8, NOTE_F4, 8,
    NOTE_F4, 2, NOTE_F4, 2, NOTE_F4, 2, NOTE_F4, 2, NOTE_F4, 2,
    
    // NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_D4, 4, NOTE_C4, 8, NOTE_F4, 2,
    // NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_A3, 4, NOTE_F4, 2,
    // NOTE_Bb3, 4, NOTE_Bb3, 4, NOTE_Bb3, 4, NOTE_Bb3, 4, NOTE_Bb3, 4, NOTE_A3, 8,

    // NOTE_C4, 4, NOTE_Bb3, 4, NOTE_A3, 2, NOTE_A3, 2, NOTE_A3, 2, NOTE_A3, 2,
    // NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4, NOTE_G3, 4,
    // NOTE_A3, 4, NOTE_G3, 4, NOTE_F3, 2, NOTE_A3, 4, NOTE_D4, 4, NOTE_F4, 2,

    // NOTE_E4, 4, NOTE_D4, 4, NOTE_C4, 4, NOTE_A3, 4, NOTE_G3, 1,
    // NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, 
    // NOTE_Bb3, 5, NOTE_A3, 4, NOTE_Bb3, 5, 
    // NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_C4, 4, NOTE_Bb3, 8, NOTE_A3, 4,

  };

  // int durations[] = {
  //   // Notes duration goes here
  //   //4, 4, 4, 4, 4, 4, 4, 8,
  //   //4, 4, 4, 4, 4, 4, 4,
  //   //4, 4, 4, 4, 8, 4, 4,
  //   //4, 4, 4, 4, 4, 4, 4,

  //   //4, 4, 4, 4, 4, 4, 4, 8,
  //   //4, 4, 4, 4, 4, 4, 4,
  //   //4, 4, 4, 4, 8, 4, 4,
  //   //4, 4, 4, 4, 4, 4, 4,

  //   //4, 4, 4, 4, 8, 4, 2,
  //   //4, 4, 4, 4, 8, 4, 2,
  //   //4, 4, 4, 4, 4, 4,
  //   //4, 4, 2, 4, 2, 8,
    
  //   //4, 4, 4, 4, 4, 4, 
  //   //4, 4, 4, 2, 2, 4, 
  //   //8, 4, 4, 4, 4, 4, 
  //   //4, 4, 4, 4, 8, 4, 

  //   //2, 4, 4, 4, 4, 4, 4, 8, 
  //   //4, 4, 4, 4, 4, 4, 8, 
  //   //4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  //   //2, 

  //   //4, 4, 4, 4, 
  //   //4, 4, 4, 4,
  //   //4, 4, 4, 4,
  //   //4, 4, 8, 8,

  //   //4, 4, 4, 4, 4, 
  //   //4, 4, 8, 8, 
  //   //2, 2, 2, 2, 2,

  //   //4, 4, 4, 4, 4, 8, 2,
  //   //4, 4, 4, 4, 4, 8, 2,
  //   //4, 4, 4, 4, 4, 8,

  //   //4, 4, 2, 2, 2, 2,
  //   //4, 4, 4, 4, 4, 4,
  //   //4, 4, 2, 4, 4, 2,

  //   4, 4, 4, 4, 1,
  //   4, 4, 4, 4, 4, 4,
  //   5, 4, 5, 
  //   4, 4, 4, 4, 4, 8, 4,

  // };

  // int size = sizeof(durations) / sizeof(int);
  int notes = sizeof(melody) / sizeof(melody[0]) / 2;
  int tempo = 180;
  int wholenote = (60000 * 4) / tempo;
  int duration = 0, noteDuration = 0;

  //for (int note = 0; note < size; note++) {
  
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
    //to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    // int duration = 1000 / durations[note];
    // tone(buzzer, melody[note], duration);

    //to distinguish the notes, set a minimum time between them.
    //the note's duration + 30% seems to work well:
    // int pauseBetweenNotes = duration * 1.30;
    // delay(pauseBetweenNotes);

    // calculates the duration of each note
    duration = melody[thisNote + 1];
    if (duration > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / duration;
    } else if (duration < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(duration);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

     // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, melody[thisNote], noteDuration * 1.3);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    //stop the tone playing:
    noTone(buzzer);
  }

}
