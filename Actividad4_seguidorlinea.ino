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

// PID configuration: This example is configured for a default
// proportional constant of 1/4 and a derivative constant of 1, which
// seems to work well at low speeds for all of our 3pi+ editions.  You
// will probably want to use trial and error to tune these constants
// for your particular 3pi+ and line course, especially if you
// increase the speed.

uint16_t proportional; // coefficient of the P term * 256
uint16_t derivative; // coefficient of the D term * 256
uint16_t integralCoefficient;

void selectP1()
{
  maxSpeed = 350;
  minSpeed = 10;
  baseSpeed = maxSpeed;
  calibrationSpeed = 70;
  proportional = 64; // P coefficient = 1/4
  derivative = 1200; // D coefficient = 1
  integralCoefficient= .25;
}

void selectP2()
{
  maxSpeed = 350;
  minSpeed = 10;
  baseSpeed = maxSpeed;
  calibrationSpeed = 70;
  proportional = 64; // P coefficient = 1/4
  derivative = 1200; // D coefficient = 1
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
  display.print(F("edition"));
  delay(1000);

  static const PololuMenuItem items[] = {
    { F("Pista 1"), selectP1 },
    { F("Pista 2"), selectP2 },
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

// Displays the estimated line position and a bar graph of sensor
// readings on the LCD. Returns after the user presses B.
void showReadings()
{
  display.clear();

  while(!buttonB.getSingleDebouncedPress())
  {
    if (color==0)
      position = lineSensors.readLineWhite(lineSensorValues);
    else
      position = lineSensors.readLineBlack(lineSensorValues);
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

void setup()
{

  bumpSensors.calibrate();

  // Uncomment if necessary to correct motor directions:
  //motors.flipLeftMotor(1);
  //motors.flipRightMotor(1);

  loadCustomCharacters();

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


  // Play music and wait for it to finish before we start driving.
  display.clear();
  display.print(F("Go!"));
  buzzer.play("L16 cdegreg4");
  
  while(buzzer.isPlaying());
}

void loop()
{
     if (bumpSensors.read() != 0) {
        motors.setSpeeds(0, 0);
        selectEdition();

      }

  // Get the position of the line.  Note that we must provide
  // the "lineSensorValues" argument to readlineWhite() here, even
  // though we are not interested in the individual sensor
  // readings.
  if (color==0)
    position = lineSensors.readLineWhite(lineSensorValues);

  else
    position = lineSensors.readLineBlack(lineSensorValues);
  // Our "error" is how far we are away from the center of the
  // line, which corresponds to position 2000.
  int16_t error = position - 2000;

  // Get motor speed difference using proportional and derivative
  // PID terms (the integral term is generally not very useful
  // for line following).
  int32_t cumulativeError = 0;
  cumulativeError += error;
  int32_t integralTerm = cumulativeError * (int32_t)integralCoefficient / 256;
  int16_t speedDifference = error * (int32_t)proportional / 256  + (error - lastError) * (int32_t)derivative / 256 + integralTerm;

  lastError = error;

  // Get individual motor speeds.  The sign of speedDifference
  // determines if the robot turns left or right.
  int16_t leftSpeed = (int16_t)baseSpeed + speedDifference;
  int16_t rightSpeed = (int16_t)baseSpeed - speedDifference;

  // Constrain our motor speeds to be between 0 and maxSpeed.
  // One motor will always be turning at maxSpeed, and the other
  // will be at maxSpeed-|speedDifference| if that is positive,
  // else it will be stationary.  For some applications, you
  // might want to allow the motor speed to go negative so that
  // it can spin in reverse.
  leftSpeed = constrain(leftSpeed, minSpeed, (int16_t)maxSpeed);
  rightSpeed = constrain(rightSpeed, minSpeed, (int16_t)maxSpeed);
  motors.setSpeeds(leftSpeed, rightSpeed);
}