/***************************************************************
**************************BEERBOY*******************************
****************************************************************
***************************V 1.0.1********************************
**********************Brewing Automation************************
****************************************************************
********************by Alberto M. Ramagini**********************
****************************************************************
*https://github.com/albeerto-dev/Beerboy
****************************************************************
Code realized on the Arduino Uno R3 (or equivalent) based on 
microprocessor ATmega328P.
This code gives you the possibility to operate on a single
Input -> single Output system to brew beer. It's perfect for 
BIAB systems, but can be used also for classic AG just by 
swapping the plug of the heating element of the mash kettle 
with the one of the boil kettle (you'll need also to move the
temp probe in the boil kettle).

Functionalities : Two modes
----> AUTO                                        
----> MANUAL

AUTO
Everything is set before the start of the brewday. In sequence
you can choose. : 1/number of mash steps 2/temperature and time
in minutes of each step of the mash 3/boil duration in minutes 
4/number of hop jetty 5/time of each hop jetty 6/hopstand 
duration in minutes.

MANUAL
It's a classic temperature PID controller. You can olny set the 
temperature desired and change it whenever you want.
****************************************************************
****************************************************************/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <PID_v1.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>



//LCD 16X2 screen inizialized 
//const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
//LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
#define I2C_ADDR 0x27 
#define BACKLIGHT_PIN 3
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7
LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

//Temperature probe inizialized
#define TEMP_PIN 13
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress thermometerAddress;

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Values to change 
//Tuning parameters
double Kp=500, Ki=0.6, Kd=0.6;
//Set to "1" if you use IMMERSION CHILLER
//Set to "0" if others
//bool setfahrenheit = 0; //if '0' celsius. If '1'fahrenheit.
bool immersionChiller = 1;
//Values to change finished

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

const float  WindowSize = 5000.0; //window size usege to calculate Output
unsigned long windowStartTime = 0; //support varible to calculate Output
const bool RELAY_PIN_ACTIVE_LEVEL = LOW; //relay state to give power to the heating element

//PIN inizialized
#define OK_PIN 8
#define UP_PIN 9
#define DOWN_PIN 10
#define SSR_PIN 11
#define BUZZER_PIN 12

//Support varibles for while cicles
bool mode = false;
bool chosen = false;
bool confirm = false;
bool hop = false;
bool mash = false;
bool boil = false;
bool hopstand = false;
bool stepOk = false;
bool validateBoil = false;
bool validateMash = false;
bool validateManual = false;
bool validateHopstand = false;
bool validateGrain = false;


//Variables used for every step of the brew
int stepCount = 1;
int hopCount = 1;
int boilCount = 60;
int hopstandCount = 0;
int tempM = 0;
double stepTemperature = 60;
int stepTime = 30;
int hopTime = 60;
int arrayHopTime[10];
int index = 0;
double manualSetPoint = 60;

struct mash{
  int time_step;
  double temp_step;
} ammostamento[10];


void setup() {
  //Pin and first start
  Serial.begin(115200);

  pinMode(TEMP_PIN, INPUT);
  pinMode(SSR_PIN, OUTPUT);
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(OK_PIN, INPUT);
  
  //probe inizialization
  sensors.begin();
  sensors.getAddress(thermometerAddress,0);
  sensors.requestTemperatures();
  sensors.setResolution(thermometerAddress,12);

  //PID inizialization
  myPID.SetOutputLimits(0, WindowSize);
  myPID.SetMode(AUTOMATIC);
  myPID.SetSampleTime(1000);
  
  //LCD inizialization 
  lcd.begin(16,2);
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  tone(BUZZER_PIN, 1000,5000);
  lcd.print("    BEERBOY");
  lcd.setCursor(0,1);
  lcd.print(" HB  Automation");
  delay(4000);
  lcd.clear();
  lcd.print("  by Alberto M.");
  lcd.setCursor(0,1);
  lcd.print("    Ramagini");
  delay(2000);
  lcd.clear();
  lcd.print("    Version ");
  lcd.setCursor(6,1);
  lcd.print("1.0");
  //tone(BUZZER_PIN, 2000,5000);
  delay(2000);
}

//Loop execution 
void loop() {
  
  lcd.clear();
  selectMode(); //first step : selcet the mode >AUTO or >MANUAL
   
  if( mode == false){ //AUTO MODE
    automatic(); //dispaly info
    stepMash(); //N. steps of the mash
    setMash(); //temperature and time of every step of the mash
    boilTime(); //boil duration in minutes
    hopJetty(); //N. of hop getty during the boil
    jetty(); //time of every getty during boil
    hopstandTime(); //hopstand duration in minutes
    start(); 
    readyToBrew(); //mash 
    boilFase(); //boil 
    hopstandFase(); //hopstand
  }
    if(mode == true){ //MANUAL MODE
    manual();
    start();
    printLCD_ManualMode(); //PID mode without time
    }
}

/*This is the most complex function. It calculates both the time and the PID
 values to have the correct Output. Every seconds the PID values are 
 calculated and the delay to calculate and display each second is only 
 240ms beacause after a deep analysis i can say that the compute_Values() 
 function has a duration of 750ms circa. In a 1 hour test the timer lost
 only 1 sec. For our purpose it's ok.
 */ 
int timer_Mash(int tempo){
  lcd.setCursor(11,1);
  int minutes = tempo-1;
  //unsigned long tempoTrascorso = 0;
  //unsigned long tempoDifferenza = 0;
  //unsigned long tempoNuovo = 0;
  int tempoAppoggio;
  int scostamento = 0;
  //Serial.println("MASH");
  while (minutes >= 0){
    int seconds = 59;
    if (minutes>=10){
    lcd.setCursor(11,1);
    lcd.print(minutes);
    }
    if(minutes<10){
      lcd.setCursor(11,1);
      lcd.print("0");
      lcd.setCursor(12,1);
      lcd.print(minutes);
    }
    lcd.setCursor(13,1);
    lcd.print(":");
      for(seconds; seconds >= 0; seconds--){
        
        //tempoTrascorso = millis();
        compute_Values();
        //tempoDifferenza = tempoTrascorso - tempoNuovo;
        //tempoNuovo = tempoTrascorso;
        //scostamento = 1000 - tempoDifferenza;
        if(seconds>=10){
          lcd.setCursor(14,1);
          lcd.print(seconds);
          delay(240);
          
        }
          if(seconds<10){
          lcd.setCursor(14,1);
          lcd.print("0");
          lcd.setCursor(15,1);
          lcd.print(seconds);
          delay(240);
         
    }   
    printLCD_Mash(index); //the display function is called to print on the LCD
  }
  minutes--; 
}
}

//Display the manual mode values on the display and calcute the PID values
void printLCD_ManualMode(){
    
    while (validateManual == false){
    Setpoint = manualSetPoint;
    compute_Values();
    lcd.setCursor(0,0);
    lcd.print("SV:");
    lcd.setCursor(3,0);
    lcd.print(Setpoint);
    lcd.setCursor(0,1);
    lcd.print("PV:");
    lcd.setCursor(3,1);
    lcd.print(Input);
    lcd.setCursor(10,0);
    lcd.print("MANUAL");
    lcd.setCursor(12,1);
    lcd.print("MODE");
    if(digitalRead(OK_PIN) == HIGH){ //press OK and set a new Setpoint
       validateManual = true;
    }
    }
    delay(2000);
    while (validateManual == true){
      manualSetPoint = constrain(manualSetPoint, 0, 100);
      tempMChange();
      lcd.setCursor(0,0);
      lcd.print("SV:");
      lcd.setCursor(3,0);
      lcd.print(manualSetPoint);
      lcd.setCursor(0,1);
      lcd.print("         ");
      lcd.setCursor(10,0);
      lcd.print("   SET");
      lcd.setCursor(12,1);
      lcd.print("TEMP");
      if(digitalRead(OK_PIN) == HIGH){ //press OK and confirm the new Setpoint
       validateManual = false;
    }
      
    }
  
}

//the mash values are printed on th LCD 
int printLCD_Mash(int indice){
    lcd.setCursor(0,0);
    lcd.print("SV:");
    lcd.setCursor(3,0);
    lcd.print(Setpoint);
    lcd.setCursor(8,0);
    lcd.print("     N#");
    lcd.setCursor(15,0);
    lcd.print(index+1);
    lcd.setCursor(0,1);
    lcd.print("PV:");
    lcd.setCursor(3,1);
    lcd.print(Input);
    lcd.setCursor(9,1);
    lcd.print("T:");
    lcd.setCursor(8,1);
    lcd.print(" ");
    /*
    Serial.print("Input : ");
    Serial.print(Input);
    Serial.print("  Setpoint : ");
    Serial.println(Setpoint);
    */
}

/*This function manages the AUTOMATIC MASH. Stepping form a step to another when 
 the setted duration is over. It is completely automatic and every step and temperature
 are respected based on the values setted before.
 */
void readyToBrew(){
  lcd.clear();

  Setpoint = ammostamento[index].temp_step;
  compute_Values();
  
  while( index < stepCount ){  
    
    if (index == 0){
      Setpoint = ammostamento[index].temp_step; //Setpoint of the specific step
      compute_Values();
      printLCD_Mash(index);
      if( (Input+1 >= Setpoint)) { //if the Input is near to setpoint insert grains
        tone(BUZZER_PIN, 2000, 5000);
        while (validateGrain == false){
         lcd.setCursor(0,0);
         lcd.print("Insert GRAINS   ");
         lcd.setCursor(0,1);
         lcd.print("Press OK to mash");
            if(digitalRead(OK_PIN) == HIGH){ //after grains IN press ok to star MASH
               lcd.clear();
               tone(BUZZER_PIN, 2000, 5000);
               timer_Mash(ammostamento[index].time_step);
               tone(BUZZER_PIN, 2000, 5000);
               validateGrain = true;
               index++;
          }
        }
      }
    }
    if(index > 0){
      Setpoint = ammostamento[index].temp_step; //Setpoint of the specific step
      compute_Values();
      printLCD_Mash(index);
        if( (Input+1 >= Setpoint)) { //if the Input is near to setpoint alarm and start count
        tone(BUZZER_PIN, 2000, 5000);
        timer_Mash(ammostamento[index].time_step);
        tone(BUZZER_PIN, 2000, 5000);
        index++;
        }
     }
      if( index == stepCount){//mash is over if BIAB remove grainbag. If AG start sparge
        while(validateMash == false ){
          Setpoint = ammostamento[index-1].temp_step;
          compute_Values();
          lcd.setCursor(0,0);
          lcd.print("Mash is OVER    ");
          lcd.setCursor(0,1);
          lcd.print("Press OK to boil");
            if(digitalRead(OK_PIN) == HIGH){ //press after sparge to start the boil fase
              tone(BUZZER_PIN, 2000, 5000);
              validateMash = true;
            }
        }
      }
   }
}


//PID values are calculuted by this function
void compute_Values(){ 
  
  unsigned long currentTime = millis();
  
  sensors.requestTemperatures();
  Input = sensors.getTempC(thermometerAddress); //Input is the probe temperature
  
  myPID.Compute(); //compute by the library
  
  if (currentTime - windowStartTime > WindowSize)
  {
    //time to shift the Relay Window
    windowStartTime += WindowSize;
  }

  // "Output" is the number of milliseconds in each
  // window to keep the output ACTIVE
  
  if (Output < (currentTime - windowStartTime))
    digitalWrite(SSR_PIN, RELAY_PIN_ACTIVE_LEVEL);  // ON
  else
     digitalWrite(SSR_PIN, !RELAY_PIN_ACTIVE_LEVEL);  // OFF

  // Display the current state every ten seconds
  const unsigned long updateInterval = 10000;
  static unsigned long updateTimer = 0;
 
 //Very useful to fix the KP, KI, KD values and have the correct Output
  if (currentTime - updateTimer <= updateInterval)
  {
    updateTimer += updateInterval;
    Serial.print("Setpoint: ");
    Serial.print(Setpoint, 1);
    Serial.print("°C, Input:");
    Serial.print(Input, 1);
    Serial.print("°C, Output:");
    Serial.print((int)Output);
    Serial.print(" milliseconds (");
    Serial.print((Output * 100.0) / WindowSize, 0);
    Serial.println("%)");
  }
  /*
  if (millis() - windowStartTime > WindowSize)
  { //time to shift the Relay Window
    windowStartTime += WindowSize;
  }
  if (Output < millis() - windowStartTime) 
    digitalWrite(SSR_PIN, HIGH);
  else 
    digitalWrite(SSR_PIN, LOW);
    */
}

//It's called when the mash is over 
void boilFase(){
  lcd.clear();
  
  sensors.requestTemperatures();
  double actual_T = sensors.getTempC(thermometerAddress);
  
  while(validateBoil == false){ //heat to boil
    
    digitalWrite(SSR_PIN, HIGH);
    
    sensors.requestTemperatures();
    double actual_T = sensors.getTempC(thermometerAddress);
    
    lcd.setCursor(0,0);
    lcd.print("Stepping to boil");
    lcd.setCursor(0,1);
    lcd.print("Temp:");
    lcd.setCursor(5,1);
    lcd.print(actual_T);
    
    while(actual_T > 96){ //boil is near press OK to start the countdown
      lcd.setCursor(0,0);
      lcd.print("OK to start BOIL");
      tone(BUZZER_PIN,5000, 5000);
       if(digitalRead(OK_PIN) == HIGH){ //countdown 
         validateBoil = true;
         break;
      }
    }
  }
  timerBoil();
}

/*First function to be called. It's a simple selection of the mode */
void selectMode(){
  lcd.clear();
  lcd.print(" Mode selection");
  lcd.setCursor(1,1);
  lcd.print("AUTO");
  lcd.setCursor(10,1);
  lcd.print("MANUAL");
  while (confirm == false){
  if(mode == false){

      lcd.setCursor(0,1);
      lcd.print(">");
      lcd.setCursor(9,1);
      lcd.print(" ");
      if((digitalRead(UP_PIN) == HIGH || digitalRead(DOWN_PIN) == HIGH) && mode == false){
        mode = true;
        delay(200);
      }
  }
  if(mode == true){
    lcd.setCursor(9,1);
    lcd.print(">");
    lcd.setCursor(0,1);
    lcd.print(" ");
    if((digitalRead(UP_PIN) == HIGH || digitalRead(DOWN_PIN) == HIGH) && mode == true){
      mode = false;
      delay(200);
    }
  }
  if(digitalRead(OK_PIN) == HIGH){
    confirm = true;
    break;
  }
  delay(100);
  }
}

//automatic mode on LCD
void automatic(){
  lcd.clear();
  lcd.print("AUTOMATIC MODE");
  lcd.setCursor(0,1);
  lcd.print("Activated");
  delay(2000);
}

//manual mode on LCD
void manual(){
  lcd.clear();
  lcd.print("MANUAL MODE ");
  lcd.setCursor(0,1);
  lcd.print("Activated");
  delay(2000);
  lcd.clear();
}

//select how many step of the mash and print on LCD
//Press OK to confirm
void stepMash(){
  while( mash == false){
  stepCount = constrain(stepCount, 0,10);
  lcd.clear();
  lcd.noCursor();
  lcd.print("N# Step mash:");
  lcd.setCursor(14,0);
  lcd.print(stepCount);
  stepChange();
  if(digitalRead(OK_PIN) == HIGH){
    mash = true;
    delay(500);
    break;
  }
  }
  
}

//Set the step of the mash based on stepMash()
//Each step has a temperature and a time
//Press OK to confirm temperature and time
void setMash(){
  lcd.clear();
  int i = 0;
  while (i<stepCount){
    int bum = 0;
    while( bum == 0) {
    stepTemperature = constrain(stepTemperature, 0, 100);
    lcd.clear();
    lcd.print("Step N#");
    lcd.setCursor(8,0);
    lcd.print(i+1);
    lcd.setCursor(0,1);
    lcd.print("Temp C: ");
    lcd.setCursor(8,1);
    lcd.print(stepTemperature);
    stepTempChange();
    if(digitalRead(OK_PIN) == HIGH){
      ammostamento[i].temp_step = stepTemperature;
      bum++;
      delay(50);
      break;
    }
  }
  while (bum == 1){
    stepTime = constrain(stepTime, 0,300);
    lcd.clear();
    lcd.print("Step N#");
    lcd.setCursor(8,0);
    lcd.print(i+1);
    lcd.setCursor(0,1);
    lcd.print("Time MIN:");
    lcd.setCursor(10,1);
    lcd.print(stepTime);
    stepTimeChange();
    if(digitalRead(OK_PIN) == HIGH){
      ammostamento[i].time_step = stepTime;
      i++;
      delay(50);
      break;
    }
    }
}
}

//Counts how many hop jetty during boil fase
void hopJetty(){
  while( hop == false){
  hopCount = constrain(hopCount,0,10);
  lcd.clear();
  lcd.print("N# Hop jetty:");
  lcd.setCursor(14,0);
  lcd.print(hopCount);
  hopChange();
  if(digitalRead(OK_PIN) == HIGH){
    hop = true;
    delay(500);
    break;
  }
  }
}

void boilTime(){
  while ( boil == false){
    boilCount = constrain(boilCount,0,300);
    lcd.clear();
    lcd.print("Boil minutes:");
    lcd.setCursor(13,0);
    lcd.print(boilCount);
    boilChange();
    if (digitalRead(OK_PIN) == HIGH){
      boil = true;
      delay(500);
      break;
    }
  }
}

void hopstandTime(){
  while( hopstand == false){
    hopstandCount = constrain(hopstandCount,0,60);
    lcd.clear();
    lcd.print("Hopstand min:");
    lcd.setCursor(14,0);
    lcd.print(hopstandCount);
    hopstandChange();
    if(digitalRead(OK_PIN) == HIGH){
      hopstand = true;
      delay(500);
      break;
    }
  }
}

void hopChange(){
  if(digitalRead(UP_PIN) == HIGH){
    hopCount++;
  }
  if(digitalRead(DOWN_PIN) == HIGH){
    hopCount--;
  }
  delay(100); 
}

void stepChange(){
   if(digitalRead(UP_PIN) == HIGH){
    stepCount++;
  }
  if(digitalRead(DOWN_PIN) == HIGH){
    stepCount--;
  }
  delay(100); 
}

void boilChange(){
  if(digitalRead(UP_PIN) == HIGH){
    boilCount++;
  }
  if(digitalRead(DOWN_PIN) == HIGH){
    boilCount--;
  }
  delay(100); 
}

void hopstandChange(){
  if(digitalRead(UP_PIN) == HIGH){
    hopstandCount++;
  }
  if(digitalRead(DOWN_PIN) == HIGH){
    hopstandCount--;
  }
  delay(100); 
}

void tempMChange(){
  if(digitalRead(UP_PIN) == HIGH){
    manualSetPoint = manualSetPoint + 0.5;
  }
 if(digitalRead(DOWN_PIN) == HIGH){
    manualSetPoint = manualSetPoint - 0.5;
  }
  delay(100); 
}

void stepTempChange(){
  if(digitalRead(UP_PIN) == HIGH){
    stepTemperature = stepTemperature + 0.5;
  }
  if(digitalRead(DOWN_PIN) == HIGH){
    stepTemperature = stepTemperature - 0.5;
  }
  delay(100); 
}

void stepTimeChange(){
  if(digitalRead(UP_PIN) == HIGH){
    stepTime++;
  }
  if(digitalRead(DOWN_PIN) == HIGH){
    stepTime--;
  }
  delay(100); 
}
void hopTimeChange(){
  if(digitalRead(UP_PIN) == HIGH){
    hopTime++;
  }
  if(digitalRead(DOWN_PIN) == HIGH){
    hopTime--;
  }
  delay(100); 
}


void start(){
  lcd.clear();
  tone(BUZZER_PIN, 1000, 1000);
  lcd.print("     START");
  delay(2000);
  lcd.clear();
}


void timerBoil(){ 
  int i = 0;
  lcd.clear();
  int minutes = boilCount-1;
  while (minutes >= 0){
    digitalWrite(SSR_PIN, HIGH);
    if( (minutes == arrayHopTime[i]-1) || arrayHopTime[i] == boilCount ){
      lcd.setCursor(0,1);
      lcd.print("HOPS IN     N#");
      lcd.setCursor(14,1);
      lcd.print(i+1);
      tone(BUZZER_PIN, 2000, 10000);
      delay(5000);
      i++;
  }
    lcd.setCursor(0,0);
    lcd.print("Countdown");
    lcd.setCursor(0,1);
    lcd.print("Wort boiling");
    int seconds = 59;
    if (minutes>=10){
    lcd.setCursor(10,0);
    lcd.print(minutes);
    }
    if(minutes<10){
      lcd.setCursor(10,0);
      lcd.print("0");
      lcd.setCursor(11,0);
      lcd.print(minutes);
    }
    lcd.setCursor(12,0);
    lcd.print(":");
      for(seconds; seconds >= 0; seconds--){
        if(seconds>=10){
          lcd.setCursor(13,0);
          lcd.print(seconds);
          delay(1000);
        }
          if(seconds<10){
          lcd.setCursor(13,0);
          lcd.print("0");
          lcd.setCursor(14,0);
          lcd.print(seconds);
          delay(1000);
    }
    
  }
  minutes--; 
  
}
digitalWrite(SSR_PIN, LOW);
lcd.clear();
lcd.print("BOIL TIME");
lcd.setCursor(0,1);
lcd.print("IS OVER");
tone(BUZZER_PIN, 2000, 5000);
delay(5000);
}

void jetty(){
  lcd.clear();
  int i = 0; 
  
  while(i < hopCount){
    hopTime = constrain(hopTime, 0,boilCount);
    lcd.clear();
    lcd.print("Jetty N#");
    lcd.setCursor(8,0);
    lcd.print(i+1);
    lcd.setCursor(0,1);
    lcd.print("Time : ");
    lcd.setCursor(6,1);
    lcd.print(hopTime);
    hopTimeChange();
    if(digitalRead(OK_PIN) == HIGH){
      arrayHopTime[i] = hopTime;
      i++;
      delay(50);
    }
    }
   
}


void hopstandFase(){
  lcd.clear();
  if(hopstandCount != 0){
  while(validateHopstand == false){
    lcd.setCursor(0,0);
    lcd.print("Press OK to");
    lcd.setCursor(0,1);
    lcd.print("hopstand");
    if(digitalRead(OK_PIN) == HIGH){
      validateHopstand = true;
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Hop stand    ");
  lcd.setCursor(0,1);
  lcd.print("Time     ");
  hopstand_CountDown();
}
  else
    coolingStep();
    //brewdayEnd();
}


void hopstand_CountDown(){
  //lcd.clear();
  int minutes = hopstandCount-1;
  while (minutes >= 0){
    int seconds = 59;
    if (minutes>=10){
    lcd.setCursor(10,1);
    lcd.print(minutes);
    }
    if(minutes<10){
      lcd.setCursor(10,1);
      lcd.print("0");
      lcd.setCursor(11,1);
      lcd.print(minutes);
    }
    lcd.setCursor(12,1);
    lcd.print(":");
      for(seconds; seconds >= 0; seconds--){
        if(seconds>=10){
          lcd.setCursor(13,1);
          lcd.print(seconds);
          delay(1000);
        }
          if(seconds<10){
          lcd.setCursor(13,1);
          lcd.print("0");
          lcd.setCursor(14,1);
          lcd.print(seconds);
          delay(1000);
    }
    
  }
  minutes--; 
  
}
coolingStep();
//brewdayEnd();
}

void coolingStep(){
  if(immersionChiller == true){
    bool finished = false;
    while(finished == false){
    sensors.requestTemperatures();
    double actualTemp = sensors.getTempC(thermometerAddress);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Chilling wort");
    lcd.setCursor(0,1);
    lcd.print("T:");
    lcd.setCursor(2,1);
    lcd.print(actualTemp);
    lcd.setCursor(8,1);
    lcd.print("°C");
    }
  }
  else
    brewdayEnd();
}

void brewdayEnd(){
  lcd.clear();
  bool finished = false;
  tone(BUZZER_PIN, 2000, 3000);
  while (finished == false){
    lcd.print("    BREWDAY   ");
    lcd.setCursor(0,1);
    lcd.print("    IS OVER      ");
  }
}
