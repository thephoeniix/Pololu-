#include <Pololu3piPlus32U4.h>
#include <string.h>

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

float velocidad = 0.0;
float distance = 0.0;
int spd=0;
int dir=0;

void setup() {
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
        velocidad = config(velocidad, opt);
        opt = 0;
        break;
      case 2:
        direction(opt);
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
  display.print("-> Velocidad         ");
  display.gotoXY(0, 5);
  display.print("Direccion         ");
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


float config(float velocidad, char opt) {
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print("Velocidad (cm/s):");
  display.gotoXY(0, 6);
  display.print(" A        B        C ");
  display.gotoXY(0, 7);
  display.print(" -        +        \7 ");
  display.display();

  while (true) {
    if (buttonA.getSingleDebouncedPress() && velocidad > 0.0) {
      velocidad = velocidad - 5.0;
    }
    else if (buttonB.getSingleDebouncedPress() && velocidad < 150.0) {
      velocidad = velocidad + 5.0;
    }

    display.gotoXY(0, 2);
    display.print("Min");
    display.gotoXY(18, 2);
    display.print("Max");
    display.gotoXY(0, 3);
    display.print(" 0 ");
    display.gotoXY(18, 3);
    display.print("150");
    display.display();
    display.gotoXY(9, 3);
    display.print(velocidad);
    display.print(" ");
    display.gotoXY(0, 2);
    display.displayPartial(2, 0, 23);
    int speed = velocidad * 400 / 150;
    motors.setSpeeds(speed, speed);
    spd=speed;

    if (buttonC.getSingleDebouncedPress()) {
      opt = 0;
      motors.setSpeeds(0, 0);
      break;
    }
  }

  return velocidad;
}


void Distance(float &distance, char opt) {
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print("Distance:         ");
  display.gotoXY(0, 6);
  display.print(" A        B        C ");
  display.gotoXY(0, 7);
  display.print(" -        +        \7 ");
  display.display();

  while (true) {
    if (buttonA.getSingleDebouncedPress() && distance > 0.0) {
      distance = distance - 5.0;
    }
    else if (buttonB.getSingleDebouncedPress()) {
      distance = distance + 5.0;
    }

    display.gotoXY(9, 3);
    display.print(distance);
    display.print(" ");
    display.gotoXY(0, 2);
    display.displayPartial(2, 0, 23);

    if (buttonC.getSingleDebouncedPress()) {
      opt = 0;
      break;
    }
  }
}

void direction(char opt) {
  display.clear();
  display.noInvert();
  display.setLayout21x8();

  display.gotoXY(0, 0);
  display.print("Direcciones:     ");
  display.gotoXY(0, 2);
  display.print("-> Adelante");
  display.gotoXY(0, 3);
  display.print("   Atras ");

  display.gotoXY(0, 7);
  display.print("Main Menu\7         :C");
  display.display();
  int op = 0;

  while (true) {
    if (buttonA.getSingleDebouncedPress()) {
      if (op < 1)
        op += 1;
      else
        op = 0;
      if (op == 0) {
        display.gotoXY(0, 2);
        display.print("-> Adelante");
        display.gotoXY(0, 3);
        display.print("   Atras");
      }
      else if (op == 1) {
        display.gotoXY(0, 2);
        display.print("   Adelante");
        display.gotoXY(0, 3);
        display.print("-> Atras");
      }
      display.display();
    }
    else if (buttonB.getSingleDebouncedPress()) {
      dir = op;
      break;
    }
    else if (buttonC.getSingleDebouncedPress()) {
      opt = 0;
      break;
    }
  }
}

void run(char opt) {
  if (dir == 0) { // Si la dirección es hacia adelante
    delay(2000);
    display.clear();
    display.print("AVANZANDO");
    display.gotoXY(0, 4); // Posición para mostrar la velocidad
    display.print("Velocidad: (cm/s)");
    display.print(velocidad);
    display.gotoXY(0, 5); // Posición para mostrar la distancia

    // Inicializar las lecturas de los encoders
    encoders.getCountsAndResetLeft();
    encoders.getCountsAndResetRight();

    // Variable para almacenar la distancia medida por los encoders
    float distancia_recorrida = 0.0;

    // Variable para almacenar la última lectura de los encoders
    int ultimo_lectura_izquierda = 0;
    int ultimo_lectura_derecha = 0;

    // Bucle principal para avanzar
    while (true) {
      // Obtener las lecturas actuales de los encoders
      int lectura_izquierda = encoders.getCountsLeft();
      int lectura_derecha = encoders.getCountsRight();

      // Calcular la distancia recorrida por cada rueda desde la última lectura
      float distancia_izquierda = (lectura_izquierda - ultimo_lectura_izquierda) * 0.028; // 0.014 cm por tick
      float distancia_derecha = (lectura_derecha - ultimo_lectura_derecha) * 0.028;

      // Calcular la distancia recorrida promedio
      float distancia_promedio = distancia_izquierda;

      // Actualizar la distancia total recorrida
      distancia_recorrida += distancia_promedio;

      // Actualizar las últimas lecturas de los encoders
      ultimo_lectura_izquierda = lectura_izquierda;
      ultimo_lectura_derecha = lectura_derecha;

      // Mostrar la distancia recorrida en la pantalla OLED
      display.gotoXY(0, 5);
      display.print("Distancia: (cm)");
      display.gotoXY(0, 6);
      display.print(distancia_recorrida);
      
      // Detenerse si se ha alcanzado la distancia deseada
      if (distancia_recorrida >= distance) {
        motors.setSpeeds(0, 0);
        break;
      }

      // Establecer la velocidad de los motores
      motors.setSpeeds(spd, spd);

      // Pequeña pausa para permitir que el robot avance
      delay(10);
    }
  } else { // Si la dirección es hacia atrás
    display.clear();
    display.print("RETROCEDIENDO");
    display.gotoXY(0, 4); // Posición para mostrar la velocidad
    display.print("Velocidad: (cm/s)");
    display.print(velocidad);
    display.gotoXY(0, 5); // Posición para mostrar la distancia

    // Inicializar las lecturas de los encoders
    encoders.getCountsAndResetLeft();
    encoders.getCountsAndResetRight();

    // Variable para almacenar la distancia medida por los encoders
    float distancia_recorrida = 0.0;

    // Variable para almacenar la última lectura de los encoders
    int ultimo_lectura_izquierda = 0;
    int ultimo_lectura_derecha = 0;

    // Bucle principal para retroceder
    while (true) {
      // Obtener las lecturas actuales de los encoders
      int lectura_izquierda = encoders.getCountsLeft();
      int lectura_derecha = encoders.getCountsRight();

      // Calcular la distancia recorrida por cada rueda desde la última lectura
      float distancia_izquierda = (lectura_izquierda - ultimo_lectura_izquierda) * 0.028; // 0.014 cm por tick
      float distancia_derecha = (lectura_derecha - ultimo_lectura_derecha) * 0.028;

      // Calcular la distancia recorrida promedio
      float distancia_promedio = distancia_izquierda;

      // Actualizar la distancia total recorrida
      distancia_recorrida += distancia_promedio;

      // Actualizar las últimas lecturas de los encoders
      ultimo_lectura_izquierda = lectura_izquierda;
      ultimo_lectura_derecha = lectura_derecha;

      // Mostrar la distancia recorrida en la pantalla OLED
      display.gotoXY(0, 5);
      display.print("Distancia: (cm)");
      display.gotoXY(0, 6);
      display.print(-distancia_recorrida); // La distancia retrocedida debe ser negativa
      
      // Detenerse si se ha alcanzado la distancia deseada
      if (-distancia_recorrida >= distance) {
        motors.setSpeeds(0, 0);
        break;
      }

      // Establecer la velocidad de los motores en reversa
      motors.setSpeeds(-spd, -spd);

      // Pequeña pausa para permitir que el robot retroceda
      delay(10);
    }
  
}





}