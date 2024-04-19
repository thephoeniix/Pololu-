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

const char backArrow[] PROGMEM = { 0b00000, 0b00010, 0b00001, 0b00101, 0b01001, 0b11110, 0b01000, 0b00100 };


float pos_left = 0;
float pos_right = 0;
float vel_left = 0;
float vel_right = 0;
float ang = 0; //de giro
float dis = 0;


float ref_ang=90;
float speed = 0;
float error = 0;
float max_speed= 100;
float min_speed = 30; 

//Ganancia del controlador
float k_dis = 1;
float k_gir = 1; 

float period = 0.05; //Periodo de muesteo en segundos
double prev_time = 0;



void setup() {
  // put your setup code here, to run once:
  display.loadCustomCharacter(backArrow, 7);
  display.setLayout21x8();
  display.init();
  display.clear();
}


void loop() {
  char opt = 0;

  while (true) {
    switch (opt) {
      case 0:
        opt = mainMenu(opt);
        break;
      case 1:
        max_speed = config(velocidad, opt);
        opt = 0;
        break;
      case 2:
        min_speed = config(velocidad, opt);
        opt = 0;
        break;
      case 3:
        Distance(distance, opt);
        opt=0;
        break;
      case 4:
        run(opt);
        opt=0;
      default:
        break;
    }
  }
}

char mainMenu(char opt) {
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print("Menu principal    ");
  display.gotoXY(0, 4);
  display.print("-> Movimientos      ");
  display.gotoXY(0, 5);
  display.print("Velocidad         ");
  display.gotoXY(0, 6);
  display.print("Distancia          ");
  display.gotoXY(0, 7);
  display.print("Run           ");
  display.display();
  int op=1;
  
    while (true) {
    if (buttonA.getSingleDebouncedPress()) {
      if (op < 4)
        op += 1;
      else
        op = 1;
      if (op == 1) {
        display.gotoXY(0, 4);
        display.print("-> Velocidad  ");
        display.gotoXY(0, 7);
        display.print("Run    ");
      }
      else if (op == 2) {
        display.gotoXY(0, 4);
        display.print("Velocidad     ");
        display.gotoXY(0, 5);
        display.print("-> Direccion     ");
      }
      else if (op == 3) {
        display.gotoXY(0, 5);
        display.print("Direccion    ");
        display.gotoXY(0, 6);
        display.print("-> Distancia     ");
      }
      else if (op == 4) {
        display.gotoXY(0, 6);
        display.print("Distancia    ");
        display.gotoXY(0, 7);
        display.print("-> Run     ");
      }
      display.display();
    }
    else if (buttonB.getSingleDebouncedPress()) {
      opt = op;
      break;
    }
  }

  

  return opt;
}

void loop() {
  //Esperamos a que haya transcurrido el periodo de muestreo
  if(millis()-prev_time>=period*1000)
  {
    //Actualizamos el tiempo anterior del ciclo
    prev_time = millis();

    //Ejecutamos nuestra rutina cada 50 ms
    
    pos_left = float(encoders.getCountsAndResetLeft()); //Pulsos del sensor
    pos_left = pos_left*(1.0/12.0); //Convertimos las unidades de pulsos a revoluciones del motor (CPR=12)
    pos_left = pos_left*(1.0/29.86); //Convertimos las unidades de revoluciones del motor a revoluciones de la rueda (29.86:1)
    pos_left = pos_left*(360); //Convertimos las unidades de revoluciones de la rueda a centimetros (diametro=3.2cm)
    pos_right = float(encoders.getCountsAndResetRight())(1.0/12.0)(1.0/29.86)*(360);
    //calculamos la distancia recorrida
    ang = ang + 1.6 * ((pos_right-pos_left)/9);

    //Control de velocidad
    //  calculo del error de posicion
    error = ref_ang - ang ;
    //ley de control
    speed = kp * error;

    //Limitamos velocidad max y min
    if (speed>max_speed){
      speed=max_speed;
    }
    if (speed<min_speed){
      speed=min_speed;
    }

    //enviamos la velocidad a los motores
    if (error>0)
      motors.setSpeeds(-speed,speed);
    else
      motors.setSpeeds(0,0);
     
    
      
    display.noAutoDisplay();
    display.clear();
    display.gotoXY(0,0);
    display.print("Reference: ");
    display.gotoXY(0,1);
    display.print(ref_ang);
    display.gotoXY(0,2);
    display.print("Angulo: ");
    display.gotoXY(0,3);
    display.print(ang);
    display.gotoXY(0,4);
    display.print("Speed: ");
    display.gotoXY(0,5);
    display.print(speed);
    display.display();
    
  }

}
