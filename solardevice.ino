/*
  keyestudio sun_follower
  lesson 11
  sun_follower
  http://www.keyestudio.com
*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <BH1750.h>
BH1750 lightMeter;

#include <dht11.h>  //include the library code:
dht11 DHT;
#define DHT11_PIN 7  //define the DHT11 as the digital pin 7

#include <Servo.h>
Servo lr_servo;  // define the name of the servo rotating right and left
Servo ud_servo;  // define the name of the servo rotating upwards and downwards

const byte buttonPin = 2;  //the pin of button;the corruption is disrupted
long buttonTimer = 0;
long longPressTime = 250;
boolean buttonActive = false;
boolean longPressActive = false;

int lr_angle = 90;  //set the initial angle to 90 degree
int ud_angle = 10;  //set the initial angle to 10 degree;keep the solar panels upright to detect the strongest light
int l_state = A0;   //define the analog voltage input of the photoresistors
int r_state = A1;
int u_state = A2;
int d_state = A3;
const byte buzzer = 6;        //set the pin of the buzzer to digital pin 6
const byte lr_servopin = 9;   //define the control signal pin of the servo rotating right and lef
const byte ud_servopin = 10;  //define the control signal pin of the servo rotating clockwise and anticlockwise

unsigned int light;   //save the variable of light intensity
byte error = 10;      //Define the error range to prevent vibration
byte m_speed = 100;    //set delay time to adjust the speed of servo;the longer the time, the smaller the speed
byte resolution = 1;  //set the rotation accuracy of the servo, the minimum rotation angle
int temperature;      //save the variable of temperature
int humidity;         //save the variable of humidity

void setup() {
  Serial.begin(9600);  //define the serial baud rate
  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin();
  lightMeter.begin();

  lr_servo.attach(lr_servopin);  // set the control pin of servo
  ud_servo.attach(ud_servopin);  // set the control pin of servo
  pinMode(l_state, INPUT);       //set the mode of pin
  pinMode(r_state, INPUT);
  pinMode(u_state, INPUT);
  pinMode(d_state, INPUT);

  pinMode(buttonPin, INPUT_PULLUP);                                               //the button pin is set to input pull-up mode
  attachInterrupt(digitalPinToInterrupt(buttonPin), adjust_resolution, CHANGE);  //xternal interrupt touch type is falling edge; adjust_resolution is interrupt service function ISR

  lcd.init();       // initialize the LCD
  lcd.backlight();  //set LCD backlight

  lr_servo.write(lr_angle);  //return to initial angle
  delay(1000);
  ud_servo.write(ud_angle);
  delay(1000);
}

void loop() {
  ServoAction();   //servo performs the action
  read_light();    //read the light intensity of bh1750
  read_dht11();    //read the value of temperature and humidity
  LcdShowValue();  //Lcd shows the values of light intensity, temperature and humidity
}

/**********the function of the servo************/
void ServoAction() {
  int L = analogRead(l_state);  //read the analog voltage value of the sensor, 0-1023
  int R = analogRead(r_state);
  int U = analogRead(u_state);
  int D = analogRead(d_state);
  /**********************system adjusting left and right序**********************/
  //  abs() is the absolute value function
  if (abs(L - R) > error && L > R) {  //Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    lr_angle -= resolution;           //reduce the angle
    //    lr_servo.attach(lr_servopin);  // connect servo
    if (lr_angle < 0) {  //limit the rotation angle of the servo
      lr_angle = 0;
    }
    lr_servo.write(lr_angle);  //output the angle of the servooutput the angle of servo
    delay(m_speed);

  } else if (abs(L - R) > error && L < R) {  //Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    lr_angle += resolution;                  //increase the angle
    //    lr_servo.attach(lr_servopin);    // connect servo
    if (lr_angle > 180) {  //limit the rotation angle of servo
      lr_angle = 180;
    }
    lr_servo.write(lr_angle);  //output the angle of servo
    delay(m_speed);

  } else if (abs(L - R) <= error) {  //Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    //    lr_servo.detach();  //release the pin of servo
    lr_servo.write(lr_angle);  //output the angle of servo
  }
  /**********************system adjusting up and down**********************/
  if (abs(U - D) > error && U >= D) {  //Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    ud_angle -= resolution;            //reduce the angle
    //    ud_servo.attach(ud_servopin);  // connect servo
    if (ud_angle < 10) {  //limit the rotation angle of servo
      ud_angle = 10;
    }
    ud_servo.write(ud_angle);  //output the angle of servo
    delay(m_speed);

  } else if (abs(U - D) > error && U < D) {  //Determine whether the error is within the acceptable range, otherwise adjust the steering gear
    ud_angle += resolution;                  //increase the angle
    //    ud_servo.attach(ud_servopin);  // connect servo
    if (ud_angle > 90) {  //limit the rotation angle of servo
      ud_angle = 90;
    }
    ud_servo.write(ud_angle);  //output the angle of servo
    delay(m_speed);

  } else if (abs(U - D) <= error) {  //Determine whether the error is within the acceptable range. If it is, keep it stable and make no change in angle
    //    ud_servo.detach();  //release the pin of servo
    ud_servo.write(ud_angle);  //output the angle of servo
  }

  //Serial monitor displays the resistance of the photoresistor and the angle of servo
  /*Serial.print(" L ");
  Serial.print(L);
  Serial.print(" R ");
  Serial.print(R);
  Serial.print("  U ");
  Serial.print(U);
  Serial.print(" D ");
  Serial.print(D);
  Serial.print("  ud_angle ");
  Serial.print(ud_angle);
  Serial.print("  lr_angle ");
  Serial.println(lr_angle);
  delay(1000);  //During the test, the serial port data is received too fast, and it can be adjusted by adding delay time 
  */
}

void LcdShowValue() {
  char str1[5];
  char str2[2];
  char str3[2];
  dtostrf(light, -5, 0, str1);  //Format the light value data as a string, left-aligned
  dtostrf(temperature, -2, 0, str2);
  dtostrf(humidity, -2, 0, str3);
  //LCD1602 display
  //display the value of the light intensity
  lcd.setCursor(0, 0);
  lcd.print("Light:");
  lcd.setCursor(6, 0);
  lcd.print(str1);
  lcd.setCursor(11, 0);
  lcd.print("lux");

  //display the value of temperature and humidity
  lcd.setCursor(0, 1);
  lcd.print((int)round(1.8 * temperature + 32));
  lcd.setCursor(2, 1);
  lcd.print("F");
  lcd.setCursor(5, 1);
  lcd.print(humidity);
  lcd.setCursor(7, 1);
  lcd.print("%");

  //show the accuracy of rotation
  lcd.setCursor(11, 1);
  lcd.print("res:");
  lcd.setCursor(15, 1);
  lcd.print(resolution);
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
  light = lightMeter.readLightLevel();  //read the light intensity detected by BH1750
}

void read_dht11() {
  int chk;
  chk = DHT.read(DHT11_PIN);  // read data
  switch (chk) {
    case DHTLIB_OK:
      break;
    case DHTLIB_ERROR_CHECKSUM:  //check and return error
      break;
    case DHTLIB_ERROR_TIMEOUT:  //Timeout and return error
      break;
    default:
      break;
  }
  temperature = DHT.temperature;
  humidity = DHT.humidity;
}

/*********function disrupts service**************/
void adjust_resolution() {
  if (digitalRead(buttonPin) == HIGH) {

		if (buttonActive == false) {
			buttonActive = true;
			buttonTimer = millis();
		}

		if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) {
			longPressActive = true;
      play_song();
		}

	} else {
		if (buttonActive == true) {
			if (longPressActive == true) {
				longPressActive = false;
			} else {
        if (resolution < 5) {
          resolution++;
        } else {
          resolution = 1;
        }
			}

			buttonActive = false;
		}

	}

  tone(buzzer, 1000, 100);
  delay(10);  //delay to eliminate vibration
  if (!digitalRead(buttonPin)) {
    if (resolution < 5) {
      resolution++;
    } else {
      resolution = 1;
    }
  }
}

void play_song() {
  #define NOTE_B0 31
  #define NOTE_C1 33
  #define NOTE_CS1 35
  #define NOTE_D1 37
  #define NOTE_DS1 39
  #define NOTE_E1 41
  #define NOTE_F1 44
  #define NOTE_FS1 46
  #define NOTE_G1 49
  #define NOTE_GS1 52
  #define NOTE_A1 55
  #define NOTE_AS1 58
  #define NOTE_B1 62
  #define NOTE_C2 65
  #define NOTE_CS2 69
  #define NOTE_D2 73
  #define NOTE_DS2 78
  #define NOTE_E2 82
  #define NOTE_F2 87
  #define NOTE_FS2 93
  #define NOTE_G2 98
  #define NOTE_GS2 104
  #define NOTE_A2 110
  #define NOTE_AS2 117
  #define NOTE_B2 123
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
  #define NOTE_C6 1047
  #define NOTE_CS6 1109
  #define NOTE_D6 1175
  #define NOTE_DS6 1245
  #define NOTE_E6 1319
  #define NOTE_F6 1397
  #define NOTE_FS6 1480
  #define NOTE_G6 1568
  #define NOTE_GS6 1661
  #define NOTE_A6 1760
  #define NOTE_AS6 1865
  #define NOTE_B6 1976
  #define NOTE_C7 2093
  #define NOTE_CS7 2217
  #define NOTE_D7 2349
  #define NOTE_DS7 2489
  #define NOTE_E7 2637
  #define NOTE_F7 2794
  #define NOTE_FS7 2960
  #define NOTE_G7 3136
  #define NOTE_GS7 3322
  #define NOTE_A7 3520
  #define NOTE_AS7 3729
  #define NOTE_B7 3951
  #define NOTE_C8 4186
  #define NOTE_CS8 4435
  #define NOTE_D8 4699
  #define NOTE_DS8 4978
  #define REST 0

  int melody[] = {
    // Notes go here
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_Bb3, NOTE_A3, REST,
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_Bb3, NOTE_A3,
    NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_C4, NOTE_Bb3,
    NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_C4, NOTE_Bb3,
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_Bb3, NOTE_A3, REST,
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_Bb3, NOTE_A3,
    NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_C4, NOTE_Bb3,
    NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_C4, NOTE_Bb3,

    NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_D4, NOTE_F4,
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_F4,
    NOTE_Bb3, NOTE_Bb3, NOTE_Bb3, NOTE_Bb3, NOTE_Bb3, NOTE_Bb3,
    NOTE_C4, NOTE_Bb3, NOTE_A3, NOTE_Bb3, NOTE_C4, NOTE_F4,
    NOTE_G3, NOTE_G3, NOTE_G3, NOTE_G3, NOTE_G3, NOTE_G3,
    NOTE_A3, NOTE_G3, NOTE_F3, NOTE_G3, NOTE_A3, NOTE_A3,
    NOTE_B3, NOTE_B3, NOTE_B3, NOTE_B3, NOTE_B3, NOTE_B3,
    NOTE_C4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_C4, REST,
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_Bb3, NOTE_A3,
    NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_Bb3, NOTE_A3,
    NOTE_C4, NOTE_D4, NOTE_F4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_D4, NOTE_F4, NOTE_G4, NOTE_A4,
    REST, 
    NOTE_A4, NOTE_G4, NOTE_F4, NOTE_D4,
    NOTE_F4, NOTE_G4, NOTE_F4, NOTE_F4,
    NOTE_G4, NOTE_A4, NOTE_C5, NOTE_C5,
    NOTE_A4, NOTE_D5, NOTE_C5, NOTE_C5,
    NOTE_A4, NOTE_A4, NOTE_A4, NOTE_F4, NOTE_F4, 
    NOTE_A4, NOTE_A4, NOTE_F4, NOTE_F4, 
    NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4,

    // A G F D  F G F F  G A C C  A D C C
    // A A A F  F A A F  F F F F  

  };

  int durations[] = {
    // Notes duration goes here
    4, 4, 4, 4, 4, 4, 4, 8,
    4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 8, 4, 4,
    4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 8,
    4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 8, 4, 4,
    4, 4, 4, 4, 4, 4, 4,

    4, 4, 4, 4, 8, 4, 2,
    4, 4, 4, 4, 8, 4, 2,
    4, 4, 4, 4, 4, 4,
    4, 4, 2, 4, 2, 8,
    4, 4, 4, 4, 4, 4, 4, 
    4, 4, 2, 2, 4, 8,
    4, 4, 4, 4, 4, 4,
    4, 4, 4, 8, 4, 2,

    4, 4, 4, 4, 4, 4, 8, 4,
    4, 4, 4, 4, 4, 8, 4, 
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    2, 
    4, 4, 4, 4, 
    4, 4, 4, 4,
    4, 4, 4, 4,
    4, 4, 8, 8,
    4, 4, 4, 4, 4, 
    4, 4, 8, 8, 
    2, 2, 2, 2,
  };

  //pinMode(BUZZER_PIN, OUTPUT);
  int size = sizeof(durations) / sizeof(int);

  for (int note = 0; note < size; note++) {
    //to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int duration = 1000 / durations[note];
    tone(buzzer, melody[note], duration);

    //to distinguish the notes, set a minimum time between them.
    //the note's duration + 30% seems to work well:
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);

    //stop the tone playing:
    noTone(buzzer);
  }

}
