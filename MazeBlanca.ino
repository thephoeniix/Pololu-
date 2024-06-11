#include <Arduino.h>

#include <Pololu3piPlus32U4.h>
using namespace Pololu3piPlus32U4;


OLED display;
Buzzer buzzer;
ButtonA buttonA;
ButtonB buttonB;
ButtonC buttonC;
LineSensors lineSensors;
BumpSensors bumpSensors;
Motors motors;
Encoders encoders;

#define NUM_SENSORS 5                        // Number of infrarred sensors
unsigned int lineSensorValues[NUM_SENSORS];  // Array used to store line sensor data

unsigned int calibrationSpeed = 80;  // Motor speed for calibration
unsigned int factorADC = 700;        // Threshold for detecting White lines

// Sensor values 
bool sensor0 = 0;
bool sensor0_last = 0;

bool sensor2 = 0;
bool sensor2_last = 0;

bool sensor4 = 0;
bool sensor4_last = 0;

int finish = 0; // Finish Flag

char path[100];
unsigned char path_length; // the length of the path

int y=0; //to use simplified path array




// Sets up special characters in the LCD so that we can display
// bar graphs.
void loadCustomCharacters(){
  static const char levels[] PROGMEM = {
    0, 0, 0, 0, 0, 0, 0, 63, 63, 63, 63, 63, 63, 63
  };
  display.loadCustomCharacter(levels + 0, 0);  // 1 bar
  display.loadCustomCharacter(levels + 1, 1);  // 2 bars
  display.loadCustomCharacter(levels + 2, 2);  // 3 bars
  display.loadCustomCharacter(levels + 3, 3);  // 4 bars
  display.loadCustomCharacter(levels + 4, 4);  // 5 bars
  display.loadCustomCharacter(levels + 5, 5);  // 6 bars
  display.loadCustomCharacter(levels + 6, 6);  // 7 bars
}

void printBar(uint8_t height){
  if (height > 8) { height = 8; }
  const char barChars[] = {' ', 0, 1, 2, 3, 4, 5, 6, (char)255};
  display.print(barChars[height]);
}

void calibrateSensors(){
  display.clear();

  // Wait 1 second and then begin automatic sensor calibration
  // by rotating in place to sweep the sensors over the line
  delay(1000);
  for(uint16_t i = 0; i < 80; i++)
  {
    if (i > 20 && i <= 60)
    {
      motors.setSpeeds(-(int16_t)calibrationSpeed, calibrationSpeed);
    }
    else
    {
      motors.setSpeeds(calibrationSpeed, -(int16_t)calibrationSpeed);
    }

    lineSensors.calibrate();
  }
  motors.setSpeeds(0, 0);
}

// Displays the estimated line position and a bar graph of sensor
// readings on the LCD. Returns after the user presses B.
void showReadings()
{
  display.clear();

  while(!buttonB.getSingleDebouncedPress())
  {
    uint16_t position = lineSensors.readLineBlack(lineSensorValues);

    display.gotoXY(0, 0);
    display.print(position);
    display.print("    ");
    display.gotoXY(0, 1);
    for (uint8_t i = 0; i < NUM_SENSORS; i++)
    {
      uint8_t barHeight = map(lineSensorValues[i], 0, 1000, 0, 8);
      printBar(barHeight);
    }

    delay(50);
  }
}

void readADC(){
    lineSensors.readLineWhite(lineSensorValues);
    sensor0 = (lineSensorValues[0] < factorADC) ? 1 : 0;
    sensor2 = (lineSensorValues[2] < factorADC) ? 1 : 0;
    sensor4 = (lineSensorValues[4] < factorADC) ? 1 : 0;
}


void follow_segment() {
  
  int16_t lastError = 0;
  uint16_t proportional = 10;  
  uint16_t derivative = 64;   
  uint16_t maxSpeed = 60;
  int16_t minSpeed = 45;
  uint16_t baseSpeed = 50;
  uint16_t position;
  int error = 0;

  readADC(); 
  sensor0_last = sensor0;
  sensor2_last = sensor2;
  sensor4_last = sensor4;

  while (1) {
    sensor0_last = sensor0;
    sensor2_last = sensor2;
    sensor4_last = sensor4;

    readADC(); 

    if((sensor0 != sensor0_last) || (sensor4 != sensor4_last) || (sensor2 != sensor2_last)){
      motors.setSpeeds(30, 30);
      delay(70);
      motors.setSpeeds(0, 0);
      break;
    }
    
    position = lineSensors.readLineWhite(lineSensorValues);
    
    error = position - 2000;

    // PD control
    int16_t speedDifference = error * (int32_t)proportional / 256 + (error - lastError) * (int32_t)derivative / 256;
    lastError = error;

    int16_t left_speed = (int16_t)baseSpeed + speedDifference;
    int16_t right_speed = (int16_t)baseSpeed - speedDifference;

    left_speed = constrain(left_speed, minSpeed, (int16_t)maxSpeed);
    right_speed = constrain(right_speed, minSpeed, (int16_t)maxSpeed);

    motors.setSpeeds(left_speed, right_speed);  

  }  
}


void turnAround() {
  motors.setSpeeds(-80, 80);
  delay(480);
  motors.setSpeeds(0, 0);
}

void turnRight() {
  motors.setSpeeds(80, -80);
  delay(250);
  motors.setSpeeds(0, 0);
}

void turnLeft() {
  motors.setSpeeds(-80, 80);
  delay(250);
  motors.setSpeeds(0, 0);
}


void decideFourWay() {
  int sensor0_last = sensor0;
  int sensor2_last = sensor2;
  int sensor4_last = sensor4;

  motors.setSpeeds(-60, 60);
  delay(45);

  readADC();

  if ((sensor0 == sensor0_last) && (sensor4 == sensor4_last) && (sensor2 == sensor2_last)) {
    motors.setSpeeds(0, 0);
    finish=1;
    return;
  }

  delay(300);
  motors.setSpeeds(0, 0);
}

// Take Decition based on sensor recorded values
void select_turn() {

    if (sensor0 == 0 && sensor2 == 0 && sensor4 == 0) { // Dead End
        display.clear();
	      display.print("Dead End");
      	turnAround();
        path[path_length]='U';
        path_length++;
    }
    else if (sensor0 == 0 && sensor2 == 0 && sensor4 == 1) { // Right turn only
        display.clear();
	      display.print("Right turn only");
      	turnRight();
    }
    else if (sensor0 == 0 && sensor2 == 1 && sensor4 == 1) { // Straight or Right
        display.clear();
        display.print("Straight");
        display.gotoXY(0,1);
        display.print("Right");
        path[path_length]='S';
        path_length++;
    }
    else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 0) { // Left turn only
        display.clear();
        display.print("Left turn");
        turnLeft();
    }
    else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 1) { // Left or Right (T)
        display.clear();
	      display.print("T");
     	  turnLeft();
        path[path_length]='L';
        path_length++;
     }
    else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 0) { // Straight or Left
        display.clear();
	      display.print("Straight");
        display.gotoXY(0,1);
        display.print("Left");
     	  turnLeft();
        path[path_length]='L';
        path_length++;
     }
    else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 1) { // Four Way
        display.clear();
	      display.print("Four Way");
      	decideFourWay();
        path[path_length]='L';
        path_length++;
    }
}

void follow_pathricio() {
  char d = path[y];
  if (sensor0 == 0 && sensor2 == 0 && sensor4 == 1) { // Right turn only
      display.clear();
      //display.print("Right turn only");
      turnRight();
  }
  else if (sensor0 == 0 && sensor2 == 1 && sensor4 == 1) { // Straight or Right
      display.clear();
      display.print("Straight");
      display.gotoXY(0,1);
      display.print("Right");
      follow_path(d);
      y++;
  }
  else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 0) { // Left turn only
      display.clear();
      //display.print("Left turn");
      turnLeft();
  }
  else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 1) { // Left or Right (T)
      display.clear();
      display.print("T");
      follow_path(d);
      y++;
    }
  else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 0) { // Straight or Left
      display.clear();
      display.print("Straight");
      display.gotoXY(0,1);
      display.print("Left");
      follow_path(d);
      y++;
    }
  else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 1) { // Four Way
      display.clear();
      int sensor0_last = sensor0;
      int sensor2_last = sensor2;
      int sensor4_last = sensor4;
      display.print("Four Way");
        
      
      delay(45);
      readADC();
      if ((sensor0 == sensor0_last) && (sensor4 == sensor4_last) && (sensor2 == sensor2_last)) {
        motors.setSpeeds(0, 0);
        finish = 3;
        return;
      }
      delay(300);
      motors.setSpeeds(0, 0);
      follow_path(d);
      y++;
      
  }
}
void follow_path(char d){
  if (d=='L')
    turnLeft();
  else if (d=='R')
    turnRight();
  else if (d=='U')
    turnAround();
  return;

}
void simplify_path(){
  for (int i = 0; i < path_length; i++){

    if(path[i] == 'U'&& path[0]!='U'){
      
      if (path[i-1] == 'L' && path[i+1] == 'L' ||
          path[i-1] == 'R' && path[i+1] == 'R')
        path[i-1]='S';

      else if (path[i-1] == 'R' && path[i+1] == 'S' ||
          path[i-1] == 'S' && path[i+1] == 'R')
        path[i-1]='L';

      else if (path[i-1] == 'L' && path[i+1] == 'S' ||
          path[i-1] == 'S' && path[i+1] == 'L')
        path[i-1]='R';

      else if (path[i-1] == 'S' && path[i+1] == 'S' ||
          path[i-1] == 'L' && path[i+1] == 'R' ||
          path[i-1] == 'R' && path[i+1] == 'L')
        path[i-1]='U';
    
      for(int j = i; j <= path_length; j++){
          path[j]=path[j+2];
          path[j+2]="";
        }    
      path_length=path_length-2;  
      i=i-2;
    }
    
  }  
}


void setup() {

  Serial.begin(9600);

  path_length = 0;

  for(int i=0;i<50;i++){
    path[i]='0';
  }
  // Set display Layout
  display.setLayout11x4(); 
  Serial.begin(9600);
  // Play a little welcome song
  buzzer.play(">g32>>c32");

  display.clear();

  display.gotoXY(0, 1);
  display.print(" PRESS B");
  display.gotoXY(0, 2);
  display.print(" TO CALIB");


  // Wait for B button press
  while (!buttonB.getSingleDebouncedPress());
  display.clear();
  calibrateSensors();
  showReadings();

  display.clear();

  display.gotoXY(0, 1);
  display.print(" PRESS B");
  display.gotoXY(0, 2);
  display.print(" TO START");
  finish=0;
  y=0;



  while (!buttonB.getSingleDebouncedPress());

   // Play music and wait for it to finish before we start driving.
  display.clear();
  buzzer.play("L16 cdegreg4");
  while(buzzer.isPlaying());
  
}

void loop() {
  if (finish==0){
    follow_segment();
    motors.setSpeeds(40, 40);
    delay(85);
    readADC();
    delay(90);
    
    // Total delay 175
    // Black track 25, 150
    motors.setSpeeds(0, 0);
    select_turn();
  }
  
  else if (finish == 1){
    finish = 2;
    display.clear();
    display.print("END OF MAZE");
    display.gotoXY(0, 1);
    buzzer.play("c16>d16>e16>f16>g16>a16>b16>c16");
    motors.setSpeeds(0, 0);    
    display.print("PRESS B");
    display.gotoXY(0, 2);
    display.print("TO RUN");
    display.gotoXY(0, 3);
    display.print("SIMPLIFIED");
    delay(1000);
    
    display.clear();
    for (int j=0;j<path_length;j++){
      display.print(path[j]);
      if(j>8)
        display.gotoXY(0, 1);
    }
      
    while (!buttonB.getSingleDebouncedPress());
    
    simplify_path();
    path[path_length-1]="";
    path_length=path_length-1;
    display.clear();
    for (int j=0;j<path_length;j++){
      display.print(path[j]);
    }
    while (!buttonB.getSingleDebouncedPress());
    
  }
  
  
  while (finish==2){
    display.clear();
     
    if (path[0]=='U'){
      turnAround();
      y++;
    }
    follow_segment();
    motors.setSpeeds(40, 40);
    delay(85);
    readADC();
    delay(90);
      

    // Total delay 175
    // Black track 25, 150
    motors.setSpeeds(0, 0);
    follow_pathricio();
  }
  if (finish==3){
  

    display.clear();
    display.print("END OF MAZE");
    display.gotoXY(0, 2);
    buzzer.play("c16>d16>e16>f16>g16>a16>b16>c16");
    motors.setSpeeds(0, 0);    
    display.print(" PRESS B");
    display.gotoXY(0, 3);
    display.print("TO RESTART");
    delay(1000);
  while (!buttonB.getSingleDebouncedPress());
  setup();
  }
  

}