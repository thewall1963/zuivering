#include <sps30.h>
#include <LiquidCrystal.h>

int ret;
// nano 3
// old bootloader ATmega 328P
// usb /dev/cu.usbserial-1410
//
// on linux stk500=recv(): erroros
// met dmesg zag ik dat ttyUSB0 steeds werd ingepikt door brltty process. met sudo apt remove brltty   
// dit verwijderd en nu heb ik ttyUSB0 aan de praat         

// Bij PM1: 40 ug schakelt de unit in en bij PM1: 10 ug gaat hij pas weer uit.

// TODO
//  Whenever there are power tools that are powered on/off the display turns blank

// Histroy || changes
//
// June 22 2021  V1.1  
// Added the lcd.begin in main loop to reset the LCD every 5 seconds to stay in nible sync
// hopefully ths prevents the LCD display from blanking when the power is switch on/off by powertools

// https://lastminuteengineers.com/arduino-1602-character-lcd-tutorial/
// https://forum.arduino.cc/t/lcd-display-16x2-showing-gibberish-how-to-reset-solved/686104/7

//
// default printout is:
// 20:52:33.605 -> PM  1.0: 9.37
// 20:52:33.605 -> PM  2.5: 9.91
// 20:52:33.605 -> PM  4.0: 9.91
// 20:52:33.605 -> PM 10.0: 9.91
// 20:52:33.605 -> NC  0.5: 64.58
// 20:52:33.641 -> NC  1.0: 74.56
// 20:52:33.641 -> NC  2.5: 74.81
// 20:52:33.675 -> NC  4.0: 74.83
// 20:52:33.675 -> NC 10.0: 74.84
// 20:52:33.710 -> Typical partical size: 0.48


// Example arduino sketch, based on 
// https://github.com/Sensirion/embedded-sps/blob/master/sps30-i2c/sps30_example_usage.c


const byte      onPin     =  2;
const byte      offPin    =  3;
const byte      relayPin  =  5;
unsigned int    timeDelay =  0;
unsigned long   countDown =  0;
unsigned long   tempTime  =  0;

unsigned long   previousMillis  = 0;        // will store last time LED was updated
const long      interval        = 5000;     // interval in milliseconds at which to probe sps30
const int       TRESHOLD        = 40;


//       L C D      D I S P L A Y

const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

unsigned int ON_TIME = 30;   //time in minutes for manual override to put the unit ON

int manualStart;
int manualStop;

int ON_Flag  = 0;
int OFF_Flag = 0;
unsigned long aanTijd = 0; //hold the time value when the manual overide ON is pressed to avoid to be swithced off if treshold is not yet met
unsigned long uitTijd = 0; //hold the time value when the manual overide OFF is pressed to avoid to be swithced on if treshold is not yet met



void setup() {
  
  //s16 ret;        // return value when probing the SPS30
  u8 auto_clean_days = 4;
  u32 auto_clean;

  lcd.begin(16, 2);

  // Clears the LCD screen
  lcd.clear();
  lcd.print(" V1.1 22 June 2021");

  delay(1000);

  pinMode(onPin, INPUT);
  pinMode(onPin, INPUT);
  pinMode(relayPin, OUTPUT);

  Serial.begin(57600);
  delay(2000);
  Serial.println("here we go");

/*  
  while (sps30_probe() != 0) {
    Serial.print("SPS sensor probing failed\n");
    lcd.print("SPS probe error");
    delay(500);
  }
*/

  Serial.print("SPS sensor probing successful\n");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  SPS probe OK  ");
  

  ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
  if (ret) {
    Serial.print("error setting the auto-clean interval: ");
    Serial.println(ret);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("error auto-clean interval: ");
    lcd.print(ret);
    delay(5000);
  }
    
  ret = sps30_start_measurement();
  if (ret < 0) {
    Serial.print("error starting measurement\n");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("error starting");
    lcd.setCursor(0,1);
    lcd.print(" measuremnt");
    delay(5000);
    
  }
  delay(1500);
  lcd.clear();
}


/////////////////////////////  M A I N  L O O P   ///////////////////////////////


void loop() {
      
  struct sps30_measurement m;
  u16 data_ready;
  //s16 ret;
  int ret;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
  
      previousMillis = currentMillis;


      lcd.begin(16,2);
  
    do {
      ret = sps30_read_data_ready(&data_ready);
      if (ret < 0) {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(" error reading ");
        lcd.setCursor(0,1);
        lcd.print("err: ");
        lcd.print(ret);
        Serial.print("error reading data-ready flag: ");
        Serial.println(ret);
      } else if (!data_ready){
        Serial.print("data not ready, no new measurement available\n");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("data not ready");
        lcd.setCursor(0,1);
        lcd.print("no new measurmnt");
      }
      else
        break;
      delay(500); 
    } while (1);
  
    ret = sps30_read_measurement(&m);
    if (ret < 0) {
      Serial.print("error reading measurement\n");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(" error reading ");
      lcd.setCursor(0,1);
      lcd.print(" measurement");
      Serial.print("error reading measurement: ");
      Serial.println(ret);
    } 
    else {
      lcd.setCursor(0,0);
      lcd.print("             ");   // clear the content for the first 13 characters to prevent sspill over from a previous read
      lcd.setCursor(0,0);
      lcd.print("PM1.0: ");
      if (m.mc_1p0>9){
        lcd.setCursor(8,0);  // to allign the output
      }
      else{
        lcd.setCursor(7,0);  // to allign the output
      }
      lcd.print(m.mc_1p0);
    }
  
    manualStart = digitalRead(onPin);   // value to hold/see if manual ON  button is pressed
    manualStop  = digitalRead(offPin);  // value to hold/see if manual OFF button is pressed
  
    if(!manualStart){
      aanTijd= millis();
      ON_Flag = 1;
      Serial.print("ON_Flag is turned to 1 and aanTijd: ");
      Serial.println(aanTijd);
    }
  
    if (!manualStop){
      uitTijd = millis();
      OFF_Flag = 1;
      Serial.print("OFF_Flag is turned to 1 and uitTijd: ");
      Serial.println(uitTijd);
      
    }
      
    if ((m.mc_1p0 > TRESHOLD) || (ON_Flag == 1)) {
      digitalWrite(relayPin, HIGH);
      lcd.setCursor(13,0);
      lcd.print("AAN");
      Serial.println("if ((m.mc_1p0 > 40) || (ON_Flag == 1))");
    }
      
    if ( (m.mc_1p0 < 10) && (ON_Flag == 0) ) {
        digitalWrite(relayPin, LOW);
        lcd.setCursor(13,0);
        lcd.print("UIT");
        OFF_Flag = 0;  
        ON_Flag  = 0;
        Serial.println("if ( (m.mc_1p0 < 10) && (ON_Flag == 0) )");
      }
    } // if (currentMillis - previousMillis >= interval)  /////////////////////////////////////////////////

  
    if ( (millis() - aanTijd) > (ON_TIME * 60000) ){   // reset the ON_flag if countdown hs finished
      ON_Flag = 0;
    }
 

    lcd.setCursor( 1,1);
    if ( ON_Flag == 1 ){
      lcd.print("ON    XXX");
    }
    else{
      lcd.print("ON    XXX  BRGT");   
    }
    
    unsigned int countDown = ((ON_TIME * 60000) - (millis() - aanTijd)) / 1000;
    
    if (( countDown % 5 == 0) && (ON_Flag == 1 )) {
      int minutes = countDown / 60 ;
      int seconds = countDown - minutes*60;
      
      if (minutes > 9)
        lcd.setCursor( 11,1);
      else{
        lcd.setCursor( 11,1);
        lcd.print(" ");
        lcd.setCursor( 12,1);
      }
      lcd.print(minutes);
      lcd.print(":");
      if (seconds<10){
        lcd.print("0");
      }
      lcd.print(seconds);
      delay(150);

      Serial.println("if (( countDown % 5 == 0) && (ON_Flag == 1 ))");
    }
}


/*
 Serial.print("minuten: ");
 Serial.print(minutes);
 Serial.print(":");
 Serial.println(seconds);
 */


/*
 * 
 *  struct sps30_measurement m;
  char serial[SPS_MAX_SERIAL_LEN];
  u16 data_ready;
  s16 ret;

  do {
    ret = sps30_read_data_ready(&data_ready);
    if (ret < 0) {
      Serial.print("error reading data-ready flag: ");
      Serial.println(ret);
    } else if (!data_ready)
      Serial.print("data not ready, no new measurement available\n");
    else
      break;
    delay(100); 
  } while (1);

  ret = sps30_read_measurement(&m);
  if (ret < 0) {
    Serial.print("error reading measurement\n");
  } else {

    Serial.print("PM  1.0: ");
    Serial.print(m.mc_1p0);
    Serial.print("      ");
    Serial.print("PM  2.5: ");
    Serial.println(m.mc_2p5);
    
    if (m.mc_1p0>9){
      digitalWrite(LED_BUILTIN,HIGH);
    }
    else {
      digitalWrite(LED_BUILTIN,LOW);
    }
    
  }


Serial.print("AAN - pin: ");
    Serial.print(manualStart);
    Serial.print("     ON_Flag  = ");
    Serial.println(ON_Flag);
    Serial.print("UIT - pin: ");
    Serial.print(manualStop);
    Serial.print("     OFF_Flag = ");
    Serial.println(OFF_Flag);



  
 */

 /*
  if (!valUp){
    if (timeDelay < 240){
      timeDelay += UP_AMOUNT ;
      lcd.setCursor(1,1);
      Serial.print("We schakelen uit over: ");
      lcd.print("Uit in ");
      lcd.print(timeDelay);
      lcd.print(" min");
      Serial.println(timeDelay);
      countDown=0;
      delay(500);
    }
    digitalWrite(relayPin, HIGH);
    
  }

  if (!valDown){
    if (timeDelay >= UP_AMOUNT){
      timeDelay -= UP_AMOUNT ;
      Serial.print("We schakelen uit over: ");
      Serial.println(timeDelay);
      countDown=0;
      lcd.setCursor(1,1);
      Serial.print("We schakelen uit over: ");
      lcd.print("Uit in ");
      lcd.print(timeDelay);
      lcd.print(" min");
      delay(500);
    } 
    
  }

  if (millis() - tempTime == 60000) {
    tempTime = millis();
    if (timeDelay >= 1){
      timeDelay -= 1;
    }  
    Serial.print("We schakelen uit over: ");
    Serial.println(timeDelay);
    lcd.setCursor(1,1);
      Serial.print("We schakelen uit over: ");
      lcd.print("Uit in ");
      lcd.print(timeDelay);
      lcd.print(" min");
  }
  
  if (timeDelay == 0)
    digitalWrite(relayPin, LOW);

*/
