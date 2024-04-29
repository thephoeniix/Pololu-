#include <Pololu3piPlus32U4.h>

using namespace Pololu3piPlus32U4;

//incluimos unicamente las partes que requerimos para esta práctica

OLED display;
ButtonA buttonA;
ButtonB buttonB;
ButtonC buttonC;
Motors motors;

//creamos una variable de velocidad inicializada en 20, que se modificará más adelante
int velocidad = 20;

//Creamos un caracter nuevo, en este caso será la flecha que indicará el regreso al menú principal
const char backArrow[] PROGMEM = {
  0b00000,
  0b00010,
  0b00001,
  0b00101,
  0b01001,
  0b11110,
  0b01000,
  0b00100,
};

void setup() {
  //cargamos el carácter neuvo
  display.loadCustomCharacter(backArrow, 7);

  display.setLayout21x8();
  display.init();
  display.clear();
}

void loop() {
  //opt será la variable principal de nuestro programa, será la variable que evaluaremos en los casos del switch
  char opt = 0;

  while (true) {
    //menu switch
    switch (opt) {
      case 0:
        //menú principal
        opt = mainMenu(opt);
        break;
      case 1:
      //config menu
        velocidad = config(velocidad, opt);
        opt = 0; // Return to main menu after configuring
        break;
      case 2:
        //Operation menu
        opt = operation(opt);
        break;
      case 3:
        //función forward
        opt = Forward();
        break;
      case 4:
        //función backward
        opt = Backward();
        break;
      case 5:
        //función clockwise
        opt = Clockwise();
        break;
      case 6:
        //función counterclockwise
        opt = CounterClockwise();
        break;
      default:
        break;
    }
  }
}

char mainMenu(char opt) {
  //main menu
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print("Menu principal    ");
  display.gotoXY(0, 5);
  display.print("Configuracion      :A");
  display.gotoXY(0, 6);
  display.print("Operacion          :B");
  display.display();

  while (true) {
    if (buttonA.getSingleDebouncedPress()) {
      //redirigimos el menú al case 1 de opt donde se ejecuta la opción marcada con la letra A, es decir la de configuración
      opt = 1;
      break;
    }
    else if (buttonB.getSingleDebouncedPress()) {
      //redirigimos la menu al case 2 donde se hace lo de opeeración
      opt = 2;
      break;
    }
  } 

  return opt;
}

int config(int velocidad, char opt) {
  //velocity
  //display velocity menu
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print("Motor Speed:         ");
  display.gotoXY(0, 6);
  display.print(" A        B        C ");
  display.gotoXY(0, 7);
  display.print(" -        +        \7 ");
  display.display();
  while (true) {

    if (buttonA.getSingleDebouncedPress() && velocidad >= 0) {
      velocidad = velocidad - 20;
    }
    else if (buttonB.getSingleDebouncedPress() && velocidad < 400) {
      velocidad = velocidad + 20;
    }
    //print vel value
    display.gotoXY(0, 2);
    display.print("Min");
    display.gotoXY(18, 2);
    display.print("Max");
    display.gotoXY(0, 3);
    display.print(" 0 ");
    display.gotoXY(18, 3);
    display.print("400");
    display.display();
    display.gotoXY(9, 3);
    display.print(velocidad);
    display.print(" ");
    display.gotoXY(0, 2);
    display.displayPartial(2, 0, 23);

    motors.setSpeeds(velocidad, velocidad);
    //option exit
    if (buttonC.getSingleDebouncedPress()) {
      opt = 0;
      motors.setSpeeds(0, 0);
      break;
    }
  }
  return velocidad;
}

char operation(char opt) {
  display.clear();
  display.noInvert();
  display.setLayout21x8();

  display.gotoXY(0, 0);
  display.print("Operation Modes:     ");
  display.gotoXY(0, 2);
  display.print("Forward");
  display.gotoXY(0, 3);
  display.print("Backward ");
  display.gotoXY(0, 4);
  display.print("Clockwise");
  display.gotoXY(0, 5);
  display.print("Counter Clockwise ");
  display.gotoXY(0, 7);
  display.print("Main Menu\7         :C");
  display.display();
  int op = 3;

  while (true) {
    if (buttonA.getSingleDebouncedPress()) {
      if (op < 6)
        op += 1;
      else
        op = 3;
      if (op == 3) {
        display.gotoXY(0, 2);
        display.print("-> Forward");
        display.gotoXY(0, 5);
        display.print("Counter Clockwise");
      }
      else if (op == 4) {
        display.gotoXY(0, 2);
        display.print("Forward");
        display.gotoXY(0, 3);
        display.print("-> Backward");
      }
      else if (op == 5) {
        display.gotoXY(0, 3);
        display.print("Backward");
        display.gotoXY(0, 4);
        display.print("-> Clockwise");
      }
      else if (op == 6) {
        display.gotoXY(0, 4);
        display.print("Clockwise");
        display.gotoXY(0, 5);
        display.print("-> Counter Clockwise");
      }
      display.display();
    }
    else if (buttonB.getSingleDebouncedPress()) {
      opt = op;
      break;
    }
    else if (buttonC.getSingleDebouncedPress()) {
      opt = 0;
      break;
    }
  }
  return opt;
}

int Forward() {
  display.clear();
  display.print("AVANZANDO");
  display.gotoXY(0, 7); // Posición para mostrar la velocidad
  display.print("Velocidad: ");
  display.print(velocidad);
  motors.setSpeeds(velocidad, velocidad);
  delay(2000);
  motors.setSpeeds(0, 0);
  return 0;
}

int Backward() {
  display.clear();
  display.print("VA PA TRAS");
  display.gotoXY(0, 7); // Posición para mostrar la velocidad
  display.print("Velocidad: ");
  display.print(velocidad);
  motors.setSpeeds(-velocidad, -velocidad);
  delay(2000);
  motors.setSpeeds(0, 0);
  return 0;
}

int Clockwise() {
  display.clear();
  display.print("CHA CHAOOO");
  display.gotoXY(0, 7); // Posición para mostrar la velocidad
  display.print("Velocidad: ");
  display.print(velocidad);
  motors.setSpeeds(velocidad, -velocidad);
  delay(2000);
  motors.setSpeeds(0, 0);
  return 0;
}

int CounterClockwise() {
  display.clear();
  display.print("CHA CHAOOO INVERSO");
  display.gotoXY(0, 7); // Posición para mostrar la velocidad
  display.print("Velocidad: ");
  display.print(velocidad);
  motors.setSpeeds(-velocidad, velocidad);
  delay(2000);
  motors.setSpeeds(0, 0);
  return 0;
}
