#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PID_v1.h>

//inizializzo la libreria associando i PIN allo schermo 
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//INIZIALIZZO LA SONDA DI TEMPERATURA
#define TEMP_PIN 13
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress thermometerAddress;

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double Kp=500, Ki=0.55, Kd=0.1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

const float  WindowSize = 5000.0;
unsigned long windowStartTime = 0;
const bool RELAY_PIN_ACTIVE_LEVEL = LOW;

//Inizializzo i PIN
#define UP_PIN 6
#define DOWN_PIN 7
#define OK_PIN 8
#define SSR_PIN 9
#define BUZZER_PIN 10


bool mode = false;
bool chosen = false;
bool confirm = false;
bool hop = false;
bool mash = false;
bool boil = false;
bool whirlpool = false;
bool stepOk = false;
bool validateBoil = false;
bool validateMash = false;
bool validateManual = false;
bool validateHopstand = false;

int stepCount = 1;
int hopCount = 1;
int boilCount = 60;
int whirlpoolCount = 0;
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
   //Inizializzo i pin e la prima accensione 
  Serial.begin(115200);

  pinMode(TEMP_PIN, INPUT);
  pinMode(SSR_PIN, OUTPUT);
  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(OK_PIN, INPUT);
  
  sensors.begin();
  sensors.getAddress(thermometerAddress,0);
  sensors.requestTemperatures();
  sensors.setResolution(thermometerAddress,12);

  myPID.SetOutputLimits(0, WindowSize);
  myPID.SetMode(AUTOMATIC);
  myPID.SetSampleTime(1000);

   
  lcd.begin(16,2);
  lcd.print("BEERBOY      2.0");
  lcd.setCursor(0,1);
  lcd.print(" by A. Ramagini");
  //tone(BUZZER_PIN, 2000,5000);
  delay(2000);
  
}

void loop() {
  
  lcd.clear();

  selectMode();
  lcd.noCursor();
 
  if( mode == false){
    automatic();
    stepMash();
    setMash();
    boilTime();
    hopGetty();
    getty();
    whirlpoolTime();
    start();
    readyToBrew();
    boilFase();
    whirlpoolFase();
  }
    if(mode == true){
    manual();
    start();
    printLCD_ManualMode();
    }
}

int timer_Mash(int tempo){
  lcd.setCursor(11,1);
  int minutes = tempo-1;
  unsigned long tempoTrascorso = 0;
  unsigned long tempoDifferenza = 0;
  unsigned long tempoNuovo = 0;
  int tempoAppoggio;
  int scostamento = 0;
  Serial.println("MASH");
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
        
        tempoTrascorso = millis();
        compute_Values();
        tempoDifferenza = tempoTrascorso - tempoNuovo;
        tempoNuovo = tempoTrascorso;
        scostamento = 1000 - tempoDifferenza;
        if(seconds>=10){
          lcd.setCursor(14,1);
          lcd.print(seconds);
          delay(250);
          
        }
          if(seconds<10){
          lcd.setCursor(14,1);
          lcd.print("0");
          lcd.setCursor(15,1);
          lcd.print(seconds);
          delay(250);
         
    }   
    printLCD_Mash(index);
  }
  minutes--; 
}
}

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
    if(digitalRead(OK_PIN) == HIGH){
       validateManual = true;
    }
    }
    delay(2000);
    while (validateManual == true){
      tempM_Up();
      tempM_Down();
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
      if(digitalRead(OK_PIN) == HIGH){
       validateManual = false;
    }
      
    }
  
}

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
    Serial.print("Input : ");
    Serial.print(Input);
    Serial.print("  Setpoint : ");
    Serial.println(Setpoint);
}


void readyToBrew(){
  lcd.clear();
  
  compute_Values();
  
  while( index < stepCount ){  
    
    Setpoint = ammostamento[index].temp_step; //il setPoint è la temperature dello specifico step
    compute_Values();
    
    printLCD_Mash(index);
    
    if( (Input+1 >= Setpoint) && (index == 0)) { //se verifica comincia il countdown
      tone(BUZZER_PIN, 2000, 3000);
      while (index == 0){
        lcd.setCursor(0,0);
        lcd.print("Insert GRAINS   ");
        lcd.setCursor(0,1);
        lcd.print("Press OK to mash");
        if(digitalRead(OK_PIN) == HIGH){
          lcd.clear();
          timer_Mash(ammostamento[index].time_step);
          break;
        }
      }
    } 
      if((Input+1 >= Setpoint) && (index!=0)){
       timer_Mash(ammostamento[index].time_step);
       index++;
      }
    }
  
    if( index == stepCount){
      while(validateMash == false ){
      Setpoint = ammostamento[index-1].temp_step;
      compute_Values();
      lcd.setCursor(0,0);
      lcd.print("Mash is OVER    ");
      lcd.setCursor(0,1);
      lcd.print("Press OK to boil");
      if(digitalRead(OK_PIN) == HIGH){
        validateMash = true;
      }
    }
   }
  
}

void compute_Values(){ //Questa funzione viene richaimata ogni volta che ho bisogno del PID
  
  unsigned long currentTime = millis();
  sensors.requestTemperatures();
  Input = sensors.getTempC(thermometerAddress);
  
  myPID.Compute();
  
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

void boilFase(){
  lcd.clear();
  sensors.requestTemperatures();
  double actual_T = sensors.getTempC(thermometerAddress);
  
  while(validateBoil == false){
    digitalWrite(SSR_PIN, HIGH);
    sensors.requestTemperatures();
    double actual_T = sensors.getTempC(thermometerAddress);
    lcd.setCursor(0,0);
    lcd.print("Stepping to boil");
    lcd.setCursor(0,1);
    lcd.print("Temp:");
    lcd.setCursor(5,1);
    lcd.print(actual_T);
    while(actual_T > 96){
      lcd.setCursor(0,0);
      lcd.print("OK to start BOIL");
      tone(BUZZER_PIN,5000, 5000);
       if(digitalRead(OK_PIN) == HIGH){
         validateBoil = true;
         break;
      }
    }
 
}
  timerBoil();
}

void selectMode(){
  lcd.clear();
  lcd.print(" Mode selection");
  while (confirm == false){
  if(mode == false){

      lcd.setCursor(0,1);
      lcd.print("     AUTO  ");
      if((digitalRead(UP_PIN) == HIGH || digitalRead(DOWN_PIN) == HIGH) && mode == false){
        mode = true;
        delay(100);
      }
  }
  if(mode == true){
    lcd.setCursor(0,1);
    lcd.print("     MANUAL");
    if((digitalRead(UP_PIN) == HIGH || digitalRead(DOWN_PIN) == HIGH) && mode == true){
      mode = false;
      delay(100);
    }
  }
  if(digitalRead(OK_PIN) == HIGH){
    confirm = true;
    break;
  }
  delay(100);
  }
}

void automatic(){
  lcd.clear();
  lcd.print("AUTOMATIC MODE");
  lcd.setCursor(0,1);
  lcd.print("Activated");
  delay(2000);
}


void manual(){
  lcd.clear();
  lcd.print("MANUAL MODE ");
  lcd.setCursor(0,1);
  lcd.print("Activated");
  delay(2000);
  lcd.clear();
}

void stepMash(){
  while( mash == false){
  lcd.clear();
  lcd.noCursor();
  lcd.print("N# Step mash:");
  lcd.setCursor(14,0);
  lcd.print(stepCount);
  stepUp();
  stepDown();
  if(digitalRead(OK_PIN) == HIGH){
    mash = true;
    delay(500);
    break;
  }
  }
  
}

void setMash(){
  lcd.clear();
  int i = 0;
  while (i<stepCount){
    int bum = 0;
    while( bum == 0) {
    lcd.clear();
    lcd.print("Step N#");
    lcd.setCursor(8,0);
    lcd.print(i+1);
    lcd.setCursor(0,1);
    lcd.print("Temp C: ");
    lcd.setCursor(8,1);
    lcd.print(stepTemperature);
    stepTemp_Up();
    stepTemp_Down();
    if(digitalRead(OK_PIN) == HIGH){
      ammostamento[i].temp_step = stepTemperature;
      bum++;
      delay(50);
      break;
    }
  }
  while (bum == 1){
    lcd.clear();
    lcd.print("Step N#");
    lcd.setCursor(8,0);
    lcd.print(i+1);
    lcd.setCursor(0,1);
    lcd.print("Time MIN:");
    lcd.setCursor(10,1);
    lcd.print(stepTime);
    stepTime_Up();
    stepTime_Down();
    if(digitalRead(OK_PIN) == HIGH){
      ammostamento[i].time_step = stepTime;
      i++;
      delay(50);
      break;
    }
    }
}
}

void hopGetty(){
  while( hop == false){
  lcd.clear();
  lcd.print("N# Hop getty:");
  lcd.setCursor(14,0);
  lcd.print(hopCount);
  hopUp();
  hopDown();
  if(digitalRead(OK_PIN) == HIGH){
    hop = true;
    delay(500);
    break;
  }
  }
}

void hopUp(){
  if(digitalRead(UP_PIN) == HIGH){
    hopCount++;
    lcd.setCursor(14,0);
    lcd.print(hopCount);
  }
  delay(50);
}

void hopDown(){
  if(digitalRead(DOWN_PIN) == HIGH){
    hopCount--;
    lcd.setCursor(14,0);
    lcd.print(hopCount);
  }
  delay(50); 
}


void stepUp(){
  if(digitalRead(UP_PIN) == HIGH){
    stepCount++;
    lcd.setCursor(14,0);
    lcd.print(stepCount);
  }
  delay(50);
}

void stepDown(){
  if(digitalRead(DOWN_PIN) == HIGH){
    stepCount--;
    lcd.setCursor(14,0);
    lcd.print(stepCount);
  }
  delay(50); 
}

void boilTime(){
  while ( boil == false){
    lcd.clear();
    lcd.print("Boil minutes:");
    lcd.setCursor(13,0);
    lcd.print(boilCount);
    boilUp();
    boilDown();
    if (digitalRead(OK_PIN) == HIGH){
      boil = true;
      delay(500);
      break;
    }
  }
}


void boilUp(){
  if(digitalRead(UP_PIN) == HIGH){
    boilCount++;
    lcd.setCursor(13,0);
    lcd.print(boilCount);
  }
  delay(50);
}

void boilDown(){
  if(digitalRead(DOWN_PIN) == HIGH){
    boilCount--;
    lcd.setCursor(13,0);
    lcd.print(boilCount);
  }
  delay(50); 
} 

void whirlpoolTime(){
  while( whirlpool == false){
    lcd.clear();
    lcd.print("Whirlpool min:");
    lcd.setCursor(14,0);
    lcd.print(whirlpoolCount);
    whirlpoolUp();
    whirlpoolDown();
    if(digitalRead(OK_PIN) == HIGH){
      whirlpool = true;
      delay(500);
      break;
    }
  }
}

void whirlpoolUp(){
  if(digitalRead(UP_PIN) == HIGH){
    whirlpoolCount++;
    lcd.setCursor(14,0);
    lcd.print(whirlpoolCount);
  }
  delay(50);
}

void whirlpoolDown(){
  if(digitalRead(DOWN_PIN) == HIGH){
    whirlpoolCount--;
    lcd.setCursor(14,0);
    lcd.print(whirlpoolCount);
  }
  delay(50); 
}

void tempM_Up(){
  if(digitalRead(UP_PIN) == HIGH){
    manualSetPoint = manualSetPoint + 0.5;
  }
  delay(50);
}

void tempM_Down(){
  if(digitalRead(DOWN_PIN) == HIGH){
    manualSetPoint = manualSetPoint - 0.5;
  }
  delay(50); 
}

void start(){
  lcd.clear();
  tone(BUZZER_PIN, 1000, 1000);
  lcd.print("     START");
  delay(2000);
  lcd.clear();
}


void timerBoil(){ 
  int i = 1;
  lcd.clear();
  int minutes = boilCount-1;
  while (minutes >= 0){
    digitalWrite(SSR_PIN, HIGH);
    if( (minutes == arrayHopTime[i]-1) || arrayHopTime[i] == boilCount ){
      lcd.setCursor(0,1);
      lcd.print("HOPS IN    N#");
      lcd.setCursor(13,1);
      lcd.print(i);
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


void stepTemp_Up(){
  if(digitalRead(UP_PIN) == HIGH){
    stepTemperature = stepTemperature + 0.5;
    lcd.setCursor(8,1);
    lcd.print(stepTemperature);
  }
  delay(50);
}

void stepTemp_Down(){
  if(digitalRead(DOWN_PIN) == HIGH){
    stepTemperature = stepTemperature - 0.5;
    lcd.setCursor(8,1);
    lcd.print(stepTemperature);
  }
  delay(50); 
}

void stepTime_Up(){
  if(digitalRead(UP_PIN) == HIGH){
    stepTime++;
    lcd.setCursor(10,1);
    lcd.print(stepTime);
  }
  delay(50);
}

void stepTime_Down(){
  if(digitalRead(DOWN_PIN) == HIGH){
    stepTime--;
    lcd.setCursor(10,1);
    lcd.print(stepTime);
  }
  delay(50); 
}

void getty(){
  lcd.clear();
  int i = 1; 
  
  while(i <= hopCount){
    lcd.clear();
    lcd.print("Getty #N");
    lcd.setCursor(8,0);
    lcd.print(i);
    lcd.setCursor(0,1);
    lcd.print("Time : ");
    lcd.setCursor(6,1);
    lcd.print(hopTime);
    hopTime_Up();
    hopTime_Down();
    if(digitalRead(OK_PIN) == HIGH){
      arrayHopTime[i] = hopTime;
      i++;
      delay(50);
    }
    }
   
}

void hopTime_Up(){
  if(digitalRead(UP_PIN) == HIGH){
    hopTime++;
    lcd.setCursor(6,1);
    lcd.print(hopTime);
  }
  delay(50);
}

void hopTime_Down(){
  if(digitalRead(DOWN_PIN) == HIGH){
    hopTime--;
    lcd.setCursor(6,1);
    lcd.print(hopTime);
  }
  delay(50); 
}


void whirlpoolFase(){
  lcd.clear();
  if(whirlpoolCount != 0){
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
  whirlpool_CountDown();
}
  else
    brewdayEnd();
}



void whirlpool_CountDown(){
  //lcd.clear();
  int minutes = whirlpoolCount-1;
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
