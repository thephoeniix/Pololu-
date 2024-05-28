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
void calibrateSensors(){
  display.clear();

  // Wait 1 second and then begin automatic sensor calibration
  // by rotating in place to sweep the sensors over the line
  delay(1000);
  for(uint16_t i = 0; i < 80; i++)
  {
    if (i > 20 && i <= 60)
    {
      motors.setSpeeds(-(int16_t)(70), (70));
    }
    else
    {
      motors.setSpeeds((70), -(int16_t)(70));
    }

    lineSensors.calibrate();
  }
  
  motors.setSpeeds(0, 0);
}

void solveMaze() {
  while(1) {
    follow_segment();
    // Drive straight a bit.  This helps us in case we entered the
    // intersection at an angle.
    // Note that we are slowing down - this prevents the robot
    // from tipping forward too much.
    motors.setSpeeds(50,50);
    delay(50);
        
    // These variables record whether the robot has seen a line to the
    // left, straight ahead, and right, while examining the current
    // intersection.
    unsigned char found_left = 0;
    unsigned char found_straight = 0;
    unsigned char found_right = 0;
        
    // Now read the sensors and check the intersection type.
    unsigned int sensors[5];
    lineSensors.readLineWhite(sensors);
        
    // Check for left and right exits.
    if(sensors[0] > 600)
      found_left = 1;
    if(sensors[4] > 600)
      found_right = 1;
        
    // Drive straight a bit more - this is enough to line up our
    // wheels with the intersection.
    motors.setSpeeds(40,40);
    delay(200);
        
    // Check for a straight exit.
    lineSensors.readLineWhite(sensors);
    if(sensors[1] > 600 || sensors[2] > 600 || sensors[3] > 600)
      found_straight = 1;
        
    // Check for the ending spot.
 
    if(sensors[1] > 600 && sensors[2] > 600 && sensors[3] > 600)
      motors.setSpeeds(0,0);
      break;
        
    // Intersection identification is complete.
    // If the maze has been solved, we can follow the existing
    // path.  Otherwise, we need to learn the solution.
    unsigned char dir = select_turn(found_left, found_straight, found_right);
        
    // Make the turn indicated by the path.
    turn(dir);
        
    // Store the intersection in the path variable.
    path[path_length] = dir;
    path_length ++;
        
    // Simplify the learned path.
    simplify_path();
        
    // Display the path on the OLED.
    //display_path();
  }
     
  // Now enter an infinite loop - we can re-run the maze as many
  // times as we want to.
  while(1) {
    // Beep to show that we finished the maze.
    
    // Wait for the user to press a button...

    for(int i = 0; i < path_length; i++) {
      follow_segment();
      // Drive straight while slowing down, as before.
      motors.setSpeeds(50,50);
      delay(50);
      motors.setSpeeds(40,40);
      delay(200);
            
      // Make a turn according to the instruction stored in
      // path[i].
      turn(path[i]);
    }
         
    // Follow the last segment up to the finish.
    follow_segment();

    // Now we should be at the finish!  Restart the loop.
  }
}

void follow_segment() {
  int last_proportional = 0;
  long integral = 0;

  while(1) {
    // Get the position of the line.
    unsigned int sensors[5];
    unsigned int position = lineSensors.readLineWhite(sensors);

    // The "proportional" term should be 0 when we are on the line.
    int proportional = ((int)position) - 2000;

    // Compute the derivative (change) and integral (sum) of the position.
    int derivative = proportional - last_proportional;
    integral += proportional;

    // Remember the last position.
    last_proportional = proportional;

    // Compute the difference between the two motor power settings,
    // m1 - m2.  If this is a positive number the robot will turn
    // to the left.  If it is a negative number, the robot will
    // turn to the right, and the magnitude of the number determines
    // the sharpness of the turn.
    int power_difference = proportional / 20 + integral / 10000 + derivative * 3 / 2;

    // Compute the actual motor settings.  We never set either motor
    // to a negative value.
    const int max = 60; // the maximum speed
    if(power_difference > max)
      power_difference = max;
    if(power_difference < -max)
      power_difference = -max;

    motors.setLeftSpeed(max + power_difference);
    motors.setRightSpeed(max - power_difference);

    // We use the inner three sensors (1, 2, and 3) for
    // determining whether there is a line straight ahead, and the
    // sensors 0 and 4 for detecting lines going to the left and
    // right.
    if(sensors[1] < 600 && sensors[2] < 600 && sensors[3] < 600 && sensors[0] < 600 && sensors[4] < 600) {
      // There is no line visible ahead, and we didn't see any
      // intersection.  Must be a dead end.
      return;
    } else if(sensors[0] > 600 || sensors[4] > 600) {
      // Found an intersection.
      return;
    }
  }
}

void turn(char dir){
    switch(dir)
    {
    case 'L':
        // Turn left.
        motors.setSpeeds(-80,80);
        delay(250);
        break;
    case 'R':
        // Turn right.
        motors.setSpeeds(80,-80);
        delay(250);
        break;
    case 'B':
        // Turn around.
        motors.setSpeeds(80,-80);
        delay(500);
        break;
    case 'S':
        // Don't do anything!
        break;
    }
}


// Path simplification.  The strategy is that whenever we encounter a
// sequence xBx, we can simplify it by cutting out the dead end.  For
// example, LBL -> S, because a single S bypasses the dead end
// represented by LBL.
void simplify_path()
{
    // only simplify the path if the second-to-last turn was a 'B'
    if(path_length < 3 || path[path_length-2] != 'B')
        return;
 
    int total_angle = 0;
    int i;
    for(i=1;i<=3;i++)
    {
        switch(path[path_length-i])
        {
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
 
    // Get the angle as a number between 0 and 360 degrees.
    total_angle = total_angle % 360;
 
    // Replace all of those turns with a single one.
    switch(total_angle)
    {
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
 
    // The path is now two steps shorter.
    path_length -= 2;
}

// This function decides which way to turn during the learning phase of
// maze solving.  It uses the variables found_left, found_straight, and
// found_right, which indicate whether there is an exit in each of the
// three directions, applying the "left hand on the wall" strategy.
char select_turn(unsigned char found_left, unsigned char found_straight,
  unsigned char found_right)
{
    // Make a decision about how to turn.  The following code
    // implements a left-hand-on-the-wall strategy, where we always
    // turn as far to the left as possible.
    if(found_left)
        return 'L';
    else if(found_straight)
        return 'S';
    else if(found_right)
        return 'R';
    else
        return 'B';
}
