#include <ezButton.h>
#include <TM1650.h>


 
#include "pitches.h" // include pitches

// Define Digital Pins
#define D_CLK 4
#define D_DIO 5
#define BUZZER 11
#define WIRE 9
#define START 2
#define STOP 10
#define MODE 3
 
TM1650 display;
// music
int songState = 0;

int melody[] = {
 NOTE_F4, NOTE_E4, NOTE_D4, NOTE_CS4,
 NOTE_C4, NOTE_B3, NOTE_AS3, NOTE_A3,
 NOTE_G3, NOTE_A3, NOTE_AS3, NOTE_A3, 
 NOTE_G3, NOTE_C4, 0, 
 
 NOTE_C4, NOTE_A3, NOTE_A3, NOTE_A3,
 NOTE_GS3, NOTE_A3, NOTE_F4, NOTE_C4, 
 NOTE_C4, NOTE_A3, NOTE_AS3, NOTE_AS3, 
 NOTE_AS3, NOTE_C4, NOTE_D4, 0, 
 
 NOTE_AS3, NOTE_G3, NOTE_G3, NOTE_G3,
 NOTE_FS3, NOTE_G3, NOTE_E4, NOTE_D4, 
 NOTE_D4, NOTE_AS3, NOTE_A3, NOTE_A3, 
 NOTE_A3, NOTE_AS3, NOTE_C4, 0,
 
 NOTE_C4, NOTE_A3, NOTE_A3, NOTE_A3,
 NOTE_GS3, NOTE_A3, NOTE_A4, NOTE_F4, 
 NOTE_F4, NOTE_C4, NOTE_B3, NOTE_G4, 
 NOTE_G4, NOTE_G4, NOTE_G4, 0,
 
 NOTE_G4, NOTE_E4, NOTE_G4, NOTE_G4,
 NOTE_FS4, NOTE_G4, NOTE_D4, NOTE_G4, 
 NOTE_G4, NOTE_FS4, NOTE_G4, NOTE_C4, 
 NOTE_B3, NOTE_C4, NOTE_B3, NOTE_C4, 0
};

int tempo[] = {
 8, 16, 8, 16,
 8, 16, 8, 16,
 16, 16, 16, 8,
 16, 8, 3,
 
 12, 16, 16, 16,
 8, 16, 8, 16,
 8, 16, 8, 16,
 8, 16, 4, 12,
 
 12, 16, 16, 16,
 8, 16, 8, 16,
 8, 16, 8, 16,
 8, 16, 4, 12,
 
 12, 16, 16, 16,
 8, 16, 8, 16,
 8, 16, 8, 16,
 8, 16, 4, 16,
 
 12, 17, 17, 17,
 8, 12, 17, 17, 
 17, 8, 16, 8,
 16, 8, 16, 8, 1 
};


// free play
int ledState = HIGH;
const long interval = 1000; 
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0; // time words last changed
const long interval1 = 1500; // interval between changing
// music
unsigned long previousMillis2 = 0; // time last changed
const long interval2 = 100; // interval between notes
 
int gameMode = 0; // keep track of game mode -- change to 0 or 1 for different modes
int gameSize = 60;
int gameScore = 9999;
int wirePenalty = gameScore/gameSize;
int countPenalty = 0;

bool runningGame = false;
bool stopGame = false;
unsigned long previousMillis3 = 0; // time last changed
const long interval3 = 1000; // interval between countdown
int count = gameSize; // challenge mode timer
char buffer[256];

ezButton button_start(START);
ezButton button_mode(MODE);
 
void showNumber(int number) {
  
    sprintf(buffer, "T %02d", number);
    Serial.println(buffer);
    display.displayString(buffer);
    display.setDot(number,true);
     
}

void displayPlayMode(int mode) {

    sprintf(buffer, "MODE %d", mode);
    Serial.println(buffer);

    if (mode==0) {
       if (display.displayRunning("    NODE TINE")) {
        while (display.displayRunningShift()) delay(300);
      } 

    }

    if (mode==1) {
       if (display.displayRunning("    NODE FREE")) {
        while (display.displayRunningShift()) delay(300);
      } 

    }
  
}
 
void buzz(int targetPin, long frequency, long length) {
    long delayValue = 1000000/frequency/2;  
    long numCycles = frequency * length/ 1000;  
    for (long i=0; i < numCycles; i++){ // for the calculated length of time...
      digitalWrite(targetPin,HIGH); // write the buzzer pin high to push out the diaphragm
      delayMicroseconds(delayValue); // wait for the calculated delay value
      digitalWrite(targetPin,LOW); // write the buzzer pin low to pull back the diaphragm
      delayMicroseconds(delayValue); // wait again for the calculated delay value
    }
}

void sing() {
    // play the song in a non blocking way
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis2 >= interval2) {
        previousMillis2 = currentMillis;
        int noteDuration = 1000 / tempo[songState];
        buzz(BUZZER, melody[songState], noteDuration);
        int pauseBetweenNotes = noteDuration;
        delay(pauseBetweenNotes);

        // stop the tone playing:
        buzz(BUZZER, 0, noteDuration);
      
        ++songState;
        // start song again if finished
        if(songState > 79) {
            songState = 14; // skip intro 
        } 
    }
}

int calculateScore(int gScore, int gSize, int wPenalty, int time_elapsed) {
     
    sprintf(buffer, ">>gameScore=%d, gameSize=%d, count=%d",  gScore, gSize, time_elapsed);
    Serial.println(buffer);
    sprintf(buffer, ">> wPenalty=%d, countPenalty=%d",   wPenalty, countPenalty );
    Serial.println(buffer);
     
    long p = 2 * 150 * (gSize-time_elapsed);
    long gamePenalty = 3 * wPenalty * countPenalty;

    if (p >= gScore) {

        return 0;
    }

    long currentScore = gScore - p;
     
    sprintf(buffer,"currentScore=%d, gamePenalty=%d", int(currentScore), int(gamePenalty));
    Serial.println(buffer);

    if (gamePenalty > currentScore){
        return 0;

    }

    currentScore = currentScore - gamePenalty;
    return int(currentScore);


}

void displayFinishScore(){
      if (gameMode == 0){
      int score = calculateScore(gameScore, gameSize, wirePenalty,   count); 
      //int score = 9999;
      sprintf(buffer, "    SCORE=%04d", score);
      if (display.displayRunning(buffer)) {
        while (display.displayRunningShift()) delay(1000);
      }
      delay(2000);
      for(int i = 0; i<10; i++){
          display.setBrightnessGradually(1);
          delay(200);
          display.setBrightnessGradually();
          delay(200);
      }
      delay(1000);
      }
      if (display.displayRunning("    !!SUPER!!PLAY AGAIN!    ")) {
          while (display.displayRunningShift()) delay(200);
        }
      delay(1000);
      displayPlayMode(gameMode);
}

void showCountdown() {
    // countdown the time remaining
    unsigned long currentMillis = millis(); // current time
    if (currentMillis - previousMillis3 >= interval3) {
        previousMillis3 = currentMillis;
        if (count == gameSize) {
          buzz(BUZZER, NOTE_A6, 1000);
        }

        if (runningGame) {
            --count;
            if (gameMode == 0)
                showNumber(count);
            if(count == 0 || stopGame == true) {
                  stopGame = false;
                  runningGame = false;
                  
                  buzz(BUZZER, NOTE_C6, 1000/24); 
                  delay(100);
                  buzz(BUZZER, NOTE_C6, 1000/24);
                  delay(100);
                  buzz(BUZZER, NOTE_C6, 1000/24);
                  displayFinishScore();
                  countPenalty = 0;
                  count = gameSize;
            }
        }
    }
}


void setup() { 
  Wire.begin(); //Join the bus as master
  display.init();
  display.displayOn();
  char line[] = "----";
  display.displayString(line);
  display.setBrightnessGradually(TM1650_MAX_BRIGHT);
 // put your setup code here, to run once:
  button_start.setDebounceTime(25);
  button_mode.setDebounceTime(25);
  pinMode(WIRE, INPUT); // setup circuit
  pinMode(BUZZER, OUTPUT); // setup buzzer 1
  pinMode(START, INPUT); // setup button
  pinMode(MODE, INPUT); // setup button
  pinMode(STOP, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("buzzwire started!");
  delay(1000);
  displayPlayMode(gameMode);
 
}

void loop() {
    button_start.loop();  
    button_mode.loop();
  
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      // set the LED with the ledState of the variable:
      digitalWrite(LED_BUILTIN, ledState);
    }
 
    if(button_start.isPressed() && runningGame == false){
            runningGame = true; // start the countdown
    }

    

    if(button_mode.isPressed()  ){
      if (runningGame == false) {
           gameMode = 1 - gameMode; // start the countdown
            displayPlayMode(gameMode);

      } else {
        if( gameMode == 1) {
            stopGame = true;
          }
      }
         
    }

   
    
    if(runningGame) {
      if(gameMode == 0 || stopGame == true) {
          showCountdown(); // advance countdown
    
      }

      if(digitalRead(STOP) == HIGH) {
          delay(25);
          if(digitalRead(STOP) == HIGH) {
                stopGame = true;
          }
      }

      if(digitalRead(WIRE) == HIGH) {
        delay(25);
        display.displayString("OUCH");
        countPenalty = countPenalty + 1;
        sprintf(buffer, "OUCH countPenalty=%d", countPenalty );
        Serial.println(buffer);
                
        if(digitalRead(WIRE) == HIGH) {
            while(digitalRead(WIRE) == HIGH) {
                buzz(BUZZER, NOTE_B0, 1000/24);
            }
        }
        display.displayString("    ");
      }
      else
        sing();
    }
}

