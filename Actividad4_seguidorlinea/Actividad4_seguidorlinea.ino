#include <Pololu3piPlus32U4.h>
#include <PololuMenu.h>

using namespace Pololu3piPlus32U4;

OLED display;
BumpSensors bumpSensors;
Buzzer buzzer;
LineSensors lineSensors;
Motors motors;
ButtonA buttonA;
ButtonB buttonB;
ButtonC buttonC;

bool color = 0;
int16_t position=0;
int16_t lastError = 0;

#define NUM_SENSORS 5
unsigned int lineSensorValues[NUM_SENSORS];

uint16_t maxSpeed;
int16_t minSpeed;
uint16_t baseSpeed;
uint16_t calibrationSpeed;

//for the cronometer
unsigned long startTime = 0; // Variable to store the start time
unsigned long elapsedTime;

uint16_t proportional; // coefficient of the P term * 256
uint16_t derivative; // coefficient of the D term * 256
uint16_t integralCoefficient;

void selectP1()
{
  maxSpeed = 300;
  minSpeed = 10;
  baseSpeed = maxSpeed;
  calibrationSpeed = 75;
  proportional = 64; // P coefficient = 1/4
  derivative = 800; // D coefficient = 1
  integralCoefficient= .25;
}

void selectP2()
{
  maxSpeed = 350;
  minSpeed = 30;
  baseSpeed = maxSpeed;
  calibrationSpeed = 70;
  proportional = 128; // P coefficient = 1/4
  derivative = 800; // D coefficient = 1
  integralCoefficient= .25;
}

void white(){
  color = 0;
}

void black(){
  color = 1;
}

PololuMenu<typeof(display)> menu;
void mode(){
  display.clear();
  display.print(F("Select"));
  display.gotoXY(0,1);
  display.print(F("mode"));
  delay(1000);

  static const PololuMenuItem items[] = {
    { F("White"), white },
    { F("Black"), black },
  };

  menu.setItems(items, 2);
  menu.setDisplay(display);
  menu.setBuzzer(buzzer);
  menu.setButtons(buttonA, buttonB, buttonC);

  while(!menu.select());

  display.gotoXY(0,1);
  display.print("OK!  ...");
}

void selectEdition()
{
  display.clear();
  display.print(F("Select"));
  display.gotoXY(0,1);
  display.print(F("Track"));
  delay(1000);

  static const PololuMenuItem items[] = {
    { F("Track 1"), selectP1 },
    { F("Track 2"), selectP2 },
  };

  menu.setItems(items, 2);
  menu.setDisplay(display);
  menu.setBuzzer(buzzer);
  menu.setButtons(buttonA, buttonB, buttonC);
  while(!menu.select());
  

  display.gotoXY(0,1);
  display.print("OK!  ...");
  delay(2000);
}

// Sets up special characters in the LCD so that we can display
// bar graphs.
void loadCustomCharacters()
{
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

void printBar(uint8_t height)
{
  if (height > 8) { height = 8; }
  const char barChars[] = {' ', 0, 1, 2, 3, 4, 5, 6, (char)255};
  display.print(barChars[height]);
}

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


void calibrateSensors()
{
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



void setup()
{
  loadCustomCharacters();

  bumpSensors.calibrate();
  // Play a little welcome song
  buzzer.play(">g32>>c32");

  mode();
  selectEdition();

  // Wait for button B to be pressed and released.
  display.clear();
  display.print(F("Press B"));
  display.gotoXY(0, 1);
  display.print(F("to calib"));
  while(!buttonB.getSingleDebouncedPress());

  calibrateSensors();
  showReadings();


  // Play music and wait for it to finish before we start driving.
  display.clear();
  display.print(F("Go!"));
  buzzer.play("L16 cdegreg4");
  
  while(buzzer.isPlaying());

  startTime = millis(); // Record the start time
  
}

void loop() {
    // Check if any bump sensor is triggered
    if (bumpSensors.read() != 0) {
        // Stop the motors
        motors.setSpeeds(0, 0);
        while(!buttonB.getSingleDebouncedPress());
        // Re-select the edition
        selectEdition();
        while(!buttonB.getSingleDebouncedPress())
        display.clear();
        display.print(F("Go!"));
        buzzer.play("L16 cdegreg4");
  
        while(buzzer.isPlaying());
        // Exit loop to prevent further actions until the robot is restarted
        startTime = millis();
        
        return;
    }

    // Calculate elapsed time
    elapsedTime = millis() - startTime;
    // Display elapsed time
    display.clear();
    display.gotoXY(0, 0);
    display.print("Time: ");
    display.gotoXY(0, 1);
    display.print(elapsedTime / 1000); // Convert milliseconds to seconds
    display.print("s");
    display.print(elapsedTime % 1000); // Convert milliseconds to seconds
    display.print("ms");

    // Read line sensors based on the selected color
    if (color == 0)
        position = lineSensors.readLineWhite(lineSensorValues);
    else
        position = lineSensors.readLineBlack(lineSensorValues);

    // Calculate error
    int16_t error = position - 2000;

    // Determine if the robot is completely off the line
    bool offLine = (position <= 10 || position >= 4000);

    // Aplicar corrección solo si no estamos en una recta
    if (!offLine) {
        // PID control para seguimiento de línea
        int32_t cumulativeError = 0;
        cumulativeError += error;
        int32_t integralTerm = cumulativeError * (int32_t)integralCoefficient / 256;
        int16_t speedDifference = error * (int32_t)proportional / 256  + (error - lastError) * (int32_t)derivative / 256 + integralTerm;

        lastError = error;

        int16_t leftSpeed = (int16_t)baseSpeed + speedDifference;
        int16_t rightSpeed = (int16_t)baseSpeed - speedDifference;

        leftSpeed = constrain(leftSpeed, minSpeed, (int16_t)maxSpeed);
        rightSpeed = constrain(rightSpeed, minSpeed, (int16_t)maxSpeed);
        motors.setSpeeds(leftSpeed, rightSpeed);
    } else {
        // Si estamos en una recta, reducir la corrección y controlar la velocidad
        int16_t leftSpeed = (int16_t)baseSpeed;
        int16_t rightSpeed = (int16_t)baseSpeed;

        // Calcular la diferencia de velocidad entre los motores para corregir la trayectoria
        int16_t turnDirection = (error > 0) ? -1 : 1;
        int16_t turnAmount = abs(error) / 100; // Ajusta este valor según la sensibilidad de tu robot
        int16_t turnSpeed = baseSpeed / 2; // Ajusta este valor para controlar la velocidad de corrección

        leftSpeed += turnDirection * turnAmount * turnSpeed;
        rightSpeed -= turnDirection * turnAmount * turnSpeed;

        leftSpeed = constrain(leftSpeed, minSpeed, (int16_t)maxSpeed);
        rightSpeed = constrain(rightSpeed, minSpeed, (int16_t)maxSpeed);
        motors.setSpeeds(leftSpeed, rightSpeed);
    }
}
