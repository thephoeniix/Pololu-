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

char path[100] = "";
unsigned char path_length = 0; // the length of the path

bool startExecution = false;

// Declaración de funciones
void setup();
void loop();
void calibrateSensors();
void solveMaze();
void follow_segment();
bool check_stop_condition();
void turn(char dir);
void simplify_path();
char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right);

void setup() {
  display.init();
  display.clear();
  display.print(F("Press B to start"));
}

void loop() {
  if (!startExecution) {
    if (buttonB.isPressed()) {
      startExecution = true;
      display.clear();
      display.print(F("Solving maze..."));
      delay(1000);
      calibrateSensors(); // Calibration before maze solving
      solveMaze();
    }
  }
}

void calibrateSensors() {
  display.clear();
  display.print(F("Calibrating..."));
  delay(1000);
  for (uint16_t i = 0; i < 80; i++) {
    if (i > 20 && i <= 60) {
      motors.setSpeeds(-(int16_t)(70), (70));
    } else {
      motors.setSpeeds((70), -(int16_t)(70));
    }
    lineSensors.calibrate();
  }
  motors.setSpeeds(0, 0);
  display.clear();
  display.print(F("Calib done"));
  delay(1000);
}

void solveMaze() {
  while (1) {
    follow_segment();
    if (check_stop_condition()) {
      return;
    }
    motors.setSpeeds(50, 50);
    delay(50);

    unsigned char found_left = 0;
    unsigned char found_straight = 0;
    unsigned char found_right = 0;

    unsigned int sensors[5];
    lineSensors.readLineWhite(sensors);

    if (sensors[0] > 600) found_left = 1;
    if (sensors[4] > 600) found_right = 1;

    motors.setSpeeds(40, 40);
    delay(200);

    lineSensors.readLineWhite(sensors);
    if (sensors[1] > 600 || sensors[2] > 600 || sensors[3] > 600) found_straight = 1;

    if (sensors[1] > 600 && sensors[2] > 600 && sensors[3] > 600) {
      motors.setSpeeds(0, 0);
      break;
    }

    unsigned char dir = select_turn(found_left, found_straight, found_right);
    turn(dir);

    path[path_length] = dir;
    path_length++;

    simplify_path();
  }

  while (1) {
    for (int i = 0; i < path_length; i++) {
      follow_segment();
      if (check_stop_condition()) {
        return;
      }
      motors.setSpeeds(50, 50);
      delay(50);
      motors.setSpeeds(40, 40);
      delay(200);
      turn(path[i]);
    }
    follow_segment();
  }
}

void follow_segment() {
  int last_proportional = 0;
  long integral = 0;

  while (1) {
    unsigned int sensors[5];
    unsigned int position = lineSensors.readLineWhite(sensors);

    int proportional = ((int)position) - 2000;
    int derivative = proportional - last_proportional;
    integral += proportional;
    last_proportional = proportional;

    int power_difference = proportional / 20 + integral / 10000 + derivative * 3 / 2;

    const int max = 60;
    if (power_difference > max) power_difference = max;
    if (power_difference < -max) power_difference = -max;

    motors.setLeftSpeed(max + power_difference);
    motors.setRightSpeed(max - power_difference);

    // Check for possible intersection or end of line
    if (sensors[0] < 600 && sensors[1] < 600 && sensors[2] < 600 && sensors[3] < 600 && sensors[4] < 600) {
      // All sensors are reading white, indicating no line is present.
      motors.setSpeeds(50, 50);  // Advance a bit more
      delay(200);  // Adjust the delay based on your needs
      if (check_stop_condition()) {
        motors.setSpeeds(0, 0);  // Stop the motors
        while (1); // Stay in an infinite loop to halt execution
      }
    } else if (sensors[0] > 600 || sensors[4] > 600) {
      // Found an intersection.
      return;
    }
  }
}

bool check_condition() {
  unsigned int sensors[5];
  lineSensors.readLineWhite(sensors);

  // Check if all sensors detect white (indicating a completely white square)
  if (sensors[0] < 600 && sensors[1] < 600 && sensors[2] < 600 && sensors[3] < 600 && sensors[4] < 600) {
    display.clear();
    display.print(F("All white"));
    return true;
  }

  // Check if we are at a T-intersection or a cross-intersection
  if ((sensors[0] > 600 || sensors[4] > 600) && (sensors[1] > 600 || sensors[2] > 600 || sensors[3] > 600)) {
    display.clear();
    display.print(F("Intersection"));
    return true;
  }

  return false;
}

void turn(char dir) {
  switch (dir) {
    case 'L':
      motors.setSpeeds(-80, 80);
      delay(250);
      break;
    case 'R':
      motors.setSpeeds(80, -80);
      delay(250);
      break;
    case 'B':
      motors.setSpeeds(80, -80);
      delay(500);
      break;
    case 'S':
      break;
  }
}

void simplify_path() {
  if (path_length < 3 || path[path_length - 2] != 'B')
    return;

  int total_angle = 0;
  for (int i = 1; i <= 3; i++) {
    switch (path[path_length - i]) {
      case 'R':
        total_angle += 90;
        break;
      case 'L':
        total_angle += 270;
        break;
      case 'B':
        total_angle += 180;
        break;
    }
  }

  total_angle %= 360;

  switch (total_angle) {
    case 0:
      path[path_length - 3] = 'S';
      break;
    case 90:
      path[path_length - 3] = 'R';
      break;
    case 180:
      path[path_length - 3] = 'B';
      break;
    case 270:
      path[path_length - 3] = 'L';
      break;
  }

  path_length -= 2;
}

char select_turn(unsigned char found_left, unsigned char found_straight, unsigned char found_right) {
  if (found_left)
    return 'L';
  else if (found_straight)
    return 'S';
  else if (found_right)
    return 'R';
  else
    return 'B';
}
