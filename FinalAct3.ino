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

const char backArrow[] PROGMEM = {0b00000, 0b00010, 0b00001, 0b00101, 0b01001, 0b11110, 0b01000, 0b00100};
const char check[] PROGMEM = {B00000, B00000, B00001, B00011, B10110, B11100, B01000, B00000};
const char downArrow[] PROGMEM = {0x04,0x04,0x04,0x04,0x1F,0x0E,0x04,0x00};

float pos_left = 0;
float pos_right = 0;
float speed = 0;
float error = 0;
float max_speed = 100;
float min_speed = 30;
float max_speed_p = max_speed * 400.0 / 150;
float min_speed_p = min_speed * 400.0 / 150;

float min_speed_ang = 10.0;
float max_speed_ang = 50.0;


float period = 0.05; // Periodo de muestreo en segundos
double prev_time = 0;

// Ganancia del controlador
float k_dis = 1.5;
float k_gir = 1.5;

// distance
float distance[4]={15.0,15.0,15.0,15.0};
float distancef = 0;
// angs
float ang[4]={90.0,90.0,90.0,90.0};

void setup() {
  bumpSensors.calibrate();
  display.loadCustomCharacter(backArrow, 7);
  display.loadCustomCharacter(check, 5);
  display.loadCustomCharacter(downArrow, 6);
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
        movimiento(distance, ang);
        opt = 0;
        break;
      case 2:
        min_speed_p = config_min(min_speed, opt);
        max_speed_p = config_max(max_speed, opt);
        k_dis = config_k(k_dis, opt);
        k_gir = config_k(k_gir, opt);
        opt = 0;
        break;
      case 3:
        run(opt);
        opt = 0;
        break;
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
  display.gotoXY(0, 2);
  display.print("-> Movimientos      ");
  display.gotoXY(0, 3);
  display.print("   Velocidad         ");
  display.gotoXY(0, 4);
  display.print("   Run           ");
  display.gotoXY(0, 6);
  display.print(" A        B");
  display.gotoXY(0, 7);
  display.print(" \6        \5");
  display.display();
  int op = 1;

  while (true) {
    if (buttonA.getSingleDebouncedPress()) {
      if (op < 3)
        op += 1;
      else
        op = 1;
      if (op == 1) {
        display.gotoXY(0, 2);
        display.print("-> Movimientos   ");
        display.gotoXY(0, 4);
        display.print("   Run      ");
      } else if (op == 2) {
        display.gotoXY(0, 2);
        display.print("   Movimientos       ");
        display.gotoXY(0, 3);
        display.print("-> Velocidad     ");
      } else if (op == 3) {
        display.gotoXY(0, 3);
        display.print("   Velocidad    ");
        display.gotoXY(0, 4);
        display.print("-> Run     ");
        
      }
      display.display();
    } else if (buttonB.getSingleDebouncedPress()) {
      opt = op;
      break;
    }
  }
  return opt;
}

void movimiento(float distance[], float ang[]) {
  // Ciclo for para iterar sobre las cuatro distancias y ángulos
  for (int i = 0; i < 4; ++i) {
    // Mostrar el mensaje correspondiente al movimiento actual
    display.clear();
    display.setLayout21x8();
    display.gotoXY(0, 0);
    display.print("Ingrese ");
    display.gotoXY(0, 1);
    display.print("Movimiento " + String(i + 1));
    display.display();
    display.gotoXY(0, 3);
    display.print("Distancia " + String(i + 1) + ":         ");
    display.gotoXY(0, 7);
    display.print(" -        +        \5 ");
    display.gotoXY(0, 5);
    display.print("Angulo " + String(i + 1) + ":");
    display.display();

    // Variables para almacenar los valores de distancia y ángulo
    float valor_distance = distance[i];
    float valor_ang = ang[i];

    // Ciclo while para ajustar la distancia y el ángulo actual
    while (true) {
  // Ajustar la distancia
      if (buttonA.getSingleDebouncedPress() && valor_distance > 0.0) {
        valor_distance -= 10.0;
      } else if (buttonB.getSingleDebouncedPress()) {
        valor_distance += 10.0;
      }
      // Mostrar los valores de distancia y ángulo en la pantalla
      display.gotoXY(13, 3);
      display.print(valor_distance);
      display.print(" *");
      display.gotoXY(13, 5);
      display.print(valor_ang);
      display.print("  ");
      display.gotoXY(0, 2);
      display.display();
      if (buttonC.getSingleDebouncedPress()) {
        break;
      }
    }

    while (true) {
    // Ajustar el ángulo
        if (buttonA.getSingleDebouncedPress() && valor_ang > 0.0) {
        valor_ang -= 45.0;
      } else if (buttonB.getSingleDebouncedPress()) {
        valor_ang += 45.0;
      }
      // Mostrar los valores de distancia y ángulo en la pantalla
      display.gotoXY(13, 3);
      display.print(valor_distance);
      display.print("  ");
      display.gotoXY(13, 5);
      display.print(valor_ang);
      display.print(" *");
      display.gotoXY(0, 2);
      display.display();
      if (buttonC.getSingleDebouncedPress()) {
        break;
      }
    }

    // Almacenar los valores de distancia y ángulo en los arreglos correspondientes
    distance[i] = valor_distance;
    ang[i] = valor_ang;
  }
}

float config_k(float k, char opt) {
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print(" Valor de k_dis/k_gir:");
  display.gotoXY(0, 6);
  display.print(" A        B        C ");
  display.gotoXY(0, 7);
  display.print(" -        +        \5 ");
  display.display();

  while (true) {
    if (buttonA.getSingleDebouncedPress() && k > 0.0) {
      k = k - 0.5;
    } else if (buttonB.getSingleDebouncedPress() && k < 5.0) {
      k = k + 0.5;
    }

    display.gotoXY(0, 2);
    display.print("Min");
    display.gotoXY(18, 2);
    display.print("Max");
    display.gotoXY(0, 3);
    display.print("0");
    display.gotoXY(18, 3);
    display.print("5");
    display.display();
    display.gotoXY(9, 3);
    display.print(k);
    display.print(" ");
    display.gotoXY(0, 2);
    display.display();
    motors.setSpeeds(speed, speed);

    if (buttonC.getSingleDebouncedPress()) {
      break;
    }
  }
  return k;
}

float config_max(float velocidad, char opt) {
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print("Velocidad max (cm/s):");
  display.gotoXY(0, 6);
  display.print(" A        B        C ");
  display.gotoXY(0, 7);
  display.print(" -        +        \5 ");
  display.display();
  float speed;
  while (true) {
    if (buttonA.getSingleDebouncedPress() && velocidad > min_speed) {
      velocidad = velocidad - 10.0;
    } else if (buttonB.getSingleDebouncedPress() && velocidad < 150.0) {
      velocidad = velocidad + 10.0;
    }

    display.gotoXY(0, 2);
    display.print("Min");
    display.gotoXY(18, 2);
    display.print("Max");
    display.gotoXY(0, 3);
    display.print(min_speed);
    display.gotoXY(18, 3);
    display.print("150");
    display.display();
    display.gotoXY(9, 3);
    display.print(velocidad);
    display.print(" ");
    display.gotoXY(0, 2);
    display.display();
    speed = velocidad * 400.0 / 150.0;
    max_speed = velocidad;
    motors.setSpeeds(speed, speed);

    if (buttonC.getSingleDebouncedPress()) {
      motors.setSpeeds(0, 0);
      break;
    }
  }

  return speed;
}

float config_min(float velocidad, char opt) {
  display.clear();
  display.setLayout21x8();
  display.gotoXY(0, 0);
  display.print("Velocidad min (cm/s):");
  display.gotoXY(0, 6);
  display.print(" A        B        C ");
  display.gotoXY(0, 7);
  display.print(" -        +        \5 ");
  display.display();
  float speed;

  while (true) {
    if (buttonA.getSingleDebouncedPress() && velocidad > 0.0) {
      velocidad = velocidad - 10.0;
    } else if (buttonB.getSingleDebouncedPress() && velocidad < max_speed) {
      velocidad = velocidad + 10.0;
    }

    display.gotoXY(0, 2);
    display.print("Min");
    display.gotoXY(18, 2);
    display.print("Max");
    display.gotoXY(0, 3);
    display.print(" 0 ");
    display.gotoXY(18, 3);
    display.print(max_speed);
    display.display();
    display.gotoXY(9, 3);
    display.print(velocidad);
    display.print(" ");
    display.gotoXY(0, 2);
    display.display();
    speed = velocidad * 400.0 / 150.0;
    motors.setSpeeds(speed, speed);
    min_speed = velocidad;

    if (buttonC.getSingleDebouncedPress()) {
      motors.setSpeeds(0, 0);
      break;
    }
  }

  return speed;
}

void run(char &opt) {
  // Iterar sobre los movimientos
  delay(1000);
  for (int i = 0; i < 4; i++) {
    distancef = 0.0;
    prev_time = millis();
    display.clear();
    display.print("Rutina " + String(i + 1));
    delay(0);

    float target_angle = ang[i]; // Ángulo objetivo
    float target_distance = distance[i]; // Distancia objetivo
    float initial_angle = 0.0; // Ángulo inicial
    float initial_distance = 0.0; // Distancia inicial
    float angle_speed = 0.0; // Velocidad de giro
    float distance_speed = 0.0; // Velocidad de avance
    float ang_temp = 0;

    // Calcular la velocidad de giro
    float angle_error = target_angle - initial_angle;
    float angle_speed_step = (max_speed_ang - min_speed_ang) / (abs(angle_error) / 1.6); // 1.6 es un factor de ajuste
    if (angle_speed_step > 0)
      angle_speed_step = min(angle_speed_step, max_speed_ang);
    else
      angle_speed_step = max(angle_speed_step, min_speed_ang);

    // Calcular la velocidad de avance
    float distance_error = target_distance - initial_distance;
    float distance_speed_step = (max_speed_p - min_speed_p) / (abs(distance_error) / 0.028); // 0.028 es la distancia por pulso
    if (distance_speed_step > 0)
      distance_speed_step = min(distance_speed_step, max_speed_p);
    else
      distance_speed_step = max(distance_speed_step, min_speed_p);

    // Bucle principal para girar el ángulo
    while (true) {
      // Verificar si se ha detectado un choque
      if (bumpSensors.read() != 0) {
        motors.setSpeeds(0, 0); // Detener los motores
        return; // Salir de la función run
      }
      
      // Calcular la velocidad de giro
      angle_error = target_angle-initial_angle;
      angle_speed = k_gir * angle_error;

      // Limitar la velocidad máxima y mínima
      angle_speed = constrain(angle_speed, min_speed_ang, max_speed_ang);

      // Establecer la velocidad de giro en los motores
      motors.setSpeeds(-angle_speed, angle_speed);

      // Actualizar el ángulo en tiempo real en la pantalla OLED
      float pos_left = float(encoders.getCountsAndResetLeft()); // Pulsos del sensor
      pos_left = pos_left * (1.0 / 12.0); // Convertimos las unidades de pulsos a revoluciones del motor (CPR=12)
      pos_left = pos_left * (1.0 / 29.86); // Convertimos las unidades de revoluciones del motor a revoluciones de la rueda (29.86:1)
      pos_left = pos_left * (360); // Convertimos las unidades de revoluciones de la rueda a centímetros (diámetro=3.2cm)
      float pos_right = float(encoders.getCountsAndResetRight()) * (1.0 / 12.0) * (1.0 / 29.86) * (360);
      // calculamos la distancia recorrida
      initial_angle = initial_angle + 1.6 * ((pos_right - pos_left) / 9);

      display.clear();
      display.print("Angulo " + String(initial_angle+3.0));

      display.display();

      // Si se ha alcanzado el ángulo objetivo, salir del bucle
      if (angle_error < 3.5)
        break;
      

      delay(20); // Esperar un breve tiempo antes de la próxima iteración
    }

    // Detener los motores de giro
    motors.setSpeeds(0, 0);

    delay(0);
    display.clear();
    display.print("Rutina " + String(i + 1));
    display.gotoXY(0, 5); // Posición para mostrar la distancia

    // Variable para almacenar la distancia medida por los encoders
    float distancia_recorrida = 0.0;

    // Variable para almacenar la última lectura de los encoders
    int ultimo_lectura_izquierda = 0;
    int ultimo_lectura_derecha = 0;


   // Bucle principal para avanzar
// Bucle principal para avanzar
// Bucle principal para avanzar
while (true) {
  // Verificar si se ha detectado un choque
  if (bumpSensors.read() != 0) {
    motors.setSpeeds(0, 0); // Detener los motores
    return; // Salir de la función run
  }
  
  if (millis() - prev_time >= period * 1000) {
    // Actualizamos el tiempo anterior del ciclo
    prev_time = millis();

    // Obtener las lecturas actuales de los encoders y calcular la distancia recorrida
    float pos_left = float(encoders.getCountsAndResetLeft()); // Pulsos del sensor
    pos_left = pos_left * (1.0 / 12.0); // Convertimos las unidades de pulsos a revoluciones del motor (CPR=12)
    pos_left = pos_left * (1.0 / 29.86); // Convertimos las unidades de revoluciones del motor a revoluciones de la rueda (29.86:1)
    pos_left = pos_left * ((3.2*3.1416)/1.0); // Convertimos las unidades de revoluciones de la rueda a grados 
    float pos_right = float(encoders.getCountsAndResetRight()) * (1.0 / 12.0) * (1.0 / 29.86) * ((3.2*3.1416)/1.0); // Calculamos la distancia recorrida del motor derecho

    // Calculamos la distancia recorrida promedio (ángulo)
    distancef = distancef +((pos_left+pos_right)/2);
    display.gotoXY(0, 5);
    display.print("Distancia: (cm)");
    display.gotoXY(0, 6);
    display.print(distancef);
    // Control de velocidad
    // Calculamos el error de posición
    error = target_distance - distancef;

    // Ley de control
    speed = k_dis * error;
     
    if (speed > max_speed) {
      speed = max_speed;
    } 
    if (speed < min_speed) {
      speed = min_speed;
    }

    // Si faltan 3 cm para llegar al destino, disminuir gradualmente la velocidad
    if (error < 5.0) {
      // Calcular la reducción gradual de la velocidad
      float remaining_distance = target_distance - distancef;
      float reduction_factor = remaining_distance; // Reducción gradual proporcional a la distancia restante
      speed = speed - reduction_factor; // Reducir la velocidad gradualmente
    }

    motors.setSpeeds(speed, speed);
    if (error < 1) {
       motors.setSpeeds(0, 0);
       break;
    }
  }
}
  }
}
