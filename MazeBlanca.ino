#include <Arduino.h>
#include <Pololu3piPlus32U4.h>
using namespace Pololu3piPlus32U4;

// Declaración de objetos para el hardware
OLED display;
Buzzer buzzer;
ButtonA buttonA;
ButtonB buttonB;
ButtonC buttonC;
LineSensors lineSensors;
BumpSensors bumpSensors;
Motors motors;
Encoders encoders;

#define NUM_SENSORS 5                        // Número de sensores infrarrojos
unsigned int lineSensorValues[NUM_SENSORS];  // Array para almacenar los datos de los sensores de línea

unsigned int calibrationSpeed = 80;  // Velocidad de los motores para la calibración
unsigned int factorADC = 700;        // Umbral para detectar líneas blancas

// Variables para valores de sensores
bool sensor0 = 0;
bool sensor0_last = 0;

bool sensor2 = 0;
bool sensor2_last = 0;

bool sensor4 = 0;
bool sensor4_last = 0;

int finish = 0; // Bandera de finalización

char path[100];
unsigned char path_length; // Longitud del camino

int y = 0; // Para usar el array de camino simplificado

// Configura caracteres especiales en la pantalla OLED para mostrar gráficos de barras.
void loadCustomCharacters() {
    static const char levels[] PROGMEM = {
        0, 0, 0, 0, 0, 0, 0, 63, 63, 63, 63, 63, 63, 63
    };
    display.loadCustomCharacter(levels + 0, 0);  // 1 barra
    display.loadCustomCharacter(levels + 1, 1);  // 2 barras
    display.loadCustomCharacter(levels + 2, 2);  // 3 barras
    display.loadCustomCharacter(levels + 3, 3);  // 4 barras
    display.loadCustomCharacter(levels + 4, 4);  // 5 barras
    display.loadCustomCharacter(levels + 5, 5);  // 6 barras
    display.loadCustomCharacter(levels + 6, 6);  // 7 barras
}

// Imprime una barra en la pantalla OLED
void printBar(uint8_t height) {
    if (height > 8) { height = 8; }
    const char barChars[] = {' ', 0, 1, 2, 3, 4, 5, 6, (char)255};
    display.print(barChars[height]);
}

// Calibra los sensores girando el robot sobre sí mismo
void calibrateSensors() {
    display.clear();

    // Espera 1 segundo y luego comienza la calibración automática
    delay(1000);
    for (uint16_t i = 0; i < 80; i++) {
        if (i > 20 && i <= 60) {
            motors.setSpeeds(-(int16_t)calibrationSpeed, calibrationSpeed);
        } else {
            motors.setSpeeds(calibrationSpeed, -(int16_t)calibrationSpeed);
        }
        lineSensors.calibrate();
    }
    motors.setSpeeds(0, 0);
}

// Muestra las lecturas de los sensores en la pantalla OLED y un gráfico de barras
void showReadings() {
    display.clear();

    while (!buttonB.getSingleDebouncedPress()) {
        uint16_t position = lineSensors.readLineBlack(lineSensorValues);

        display.gotoXY(0, 0);
        display.print(position);
        display.print("    ");
        display.gotoXY(0, 1);
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            uint8_t barHeight = map(lineSensorValues[i], 0, 1000, 0, 8);
            printBar(barHeight);
        }
        delay(50);
    }
}

// Lee los valores de los sensores y actualiza las variables de sensor
void readADC() {
    lineSensors.readLineWhite(lineSensorValues);
    sensor0 = (lineSensorValues[0] < factorADC) ? 1 : 0;
    sensor2 = (lineSensorValues[2] < factorADC) ? 1 : 0;
    sensor4 = (lineSensorValues[4] < factorADC) ? 1 : 0;
}

// Sigue un segmento de la línea usando control PD
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

        // Verifica si hay cambios en los sensores
        if ((sensor0 != sensor0_last) || (sensor4 != sensor4_last) || (sensor2 != sensor2_last)) {
            motors.setSpeeds(30, 30);
            delay(70);
            motors.setSpeeds(0, 0);
            break;
        }

        position = lineSensors.readLineWhite(lineSensorValues);
        error = position - 2000;

        // Control PD
        int16_t speedDifference = error * (int32_t)proportional / 256 + (error - lastError) * (int32_t)derivative / 256;
        lastError = error;

        int16_t left_speed = (int16_t)baseSpeed + speedDifference;
        int16_t right_speed = (int16_t)baseSpeed - speedDifference;

        left_speed = constrain(left_speed, minSpeed, (int16_t)maxSpeed);
        right_speed = constrain(right_speed, minSpeed, (int16_t)maxSpeed);

        motors.setSpeeds(left_speed, right_speed);
    }
}

// Realiza un giro en U
void turnAround() {
    motors.setSpeeds(-80, 80);
    delay(480);
    motors.setSpeeds(0, 0);
}

// Realiza un giro a la derecha
void turnRight() {
    motors.setSpeeds(80, -80);
    delay(250);
    motors.setSpeeds(0, 0);
}

// Realiza un giro a la izquierda
void turnLeft() {
    motors.setSpeeds(-80, 80);
    delay(250);
    motors.setSpeeds(0, 0);
}

// Decide el giro en una intersección de cuatro caminos
void decideFourWay() {
    int sensor0_last = sensor0;
    int sensor2_last = sensor2;
    int sensor4_last = sensor4;

    motors.setSpeeds(-60, 60);
    delay(45);

    readADC();

    if ((sensor0 == sensor0_last) && (sensor4 == sensor4_last) && (sensor2 == sensor2_last)) {
        motors.setSpeeds(0, 0);
        finish = 1;
        return;
    }

    delay(300);
    motors.setSpeeds(0, 0);
}

// Toma una decisión de giro basada en los valores de los sensores
void select_turn() {
    if (sensor0 == 0 && sensor2 == 0 && sensor4 == 0) { // Fin de camino
        display.clear();
        display.print("Dead End");
        turnAround();
        path[path_length] = 'U';
        path_length++;
    } else if (sensor0 == 0 && sensor2 == 0 && sensor4 == 1) { // Solo giro a la derecha
        display.clear();
        display.print("Right turn only");
        turnRight();
    } else if (sensor0 == 0 && sensor2 == 1 && sensor4 == 1) { // Recto o derecha
        display.clear();
        display.print("Straight");
        display.gotoXY(0, 1);
        display.print("Right");
        path[path_length] = 'S';
        path_length++;
    } else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 0) { // Solo giro a la izquierda
        display.clear();
        display.print("Left turn");
        turnLeft();
    } else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 1) { // Izquierda o derecha (T)
        display.clear();
        display.print("T");
        turnLeft();
        path[path_length] = 'L';
        path_length++;
    } else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 0) { // Recto o izquierda
        display.clear();
        display.print("Straight");
        display.gotoXY(0, 1);
        display.print("Left");
        turnLeft();
        path[path_length] = 'L';
        path_length++;
    } else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 1) { // Intersección de cuatro caminos
        display.clear();
        display.print("Four Way");
        decideFourWay();
        path[path_length] = 'L';
        path_length++;
    }
}

// Sigue el camino simplificado
void follow_pathricio() {
    char d = path[y];
    if (sensor0 == 0 && sensor2 == 0 && sensor4 == 1) { // Solo giro a la derecha
        display.clear();
        turnRight();
    } else if (sensor0 == 0 && sensor2 == 1 && sensor4 == 1) { // Recto o derecha
        display.clear();
        display.print("Straight");
        display.gotoXY(0, 1);
        display.print("Right");
        follow_path(d);
        y++;
    } else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 0) { // Solo giro a la izquierda
        display.clear();
        turnLeft();
    } else if (sensor0 == 1 && sensor2 == 0 && sensor4 == 1) { // Izquierda o derecha (T)
        display.clear();
        display.print("T");
        follow_path(d);
        y++;
    } else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 0) { // Recto o izquierda
        display.clear();
        display.print("Straight");
        display.gotoXY(0, 1);
        display.print("Left");
        follow_path(d);
        y++;
    } else if (sensor0 == 1 && sensor2 == 1 && sensor4 == 1) { // Intersección de cuatro caminos
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

// Sigue el camino basado en el carácter proporcionado
void follow_path(char d) {
    if (d == 'L') {
        turnLeft();
    } else if (d == 'R') {
        turnRight();
    } else if (d == 'U') {
        turnAround();
    }
    return;
}

// Simplifica el camino registrado
void simplify_path() {
    for (int i = 0; i < path_length; i++) {
        if (path[i] == 'U' && path[0] != 'U') {
            if (path[i-1] == 'L' && path[i+1] == 'L' ||
                path[i-1] == 'R' && path[i+1] == 'R') {
                path[i-1] = 'S';
            } else if (path[i-1] == 'R' && path[i+1] == 'S' ||
                       path[i-1] == 'S' && path[i+1] == 'R') {
                path[i-1] = 'L';
            } else if (path[i-1] == 'L' && path[i+1] == 'S' ||
                       path[i-1] == 'S' && path[i+1] == 'L') {
                path[i-1] = 'R';
            } else if (path[i-1] == 'S' && path[i+1] == 'S' ||
                       path[i-1] == 'L' && path[i+1] == 'R' ||
                       path[i-1] == 'R' && path[i+1] == 'L') {
                path[i-1] = 'U';
            }
            for (int j = i; j <= path_length; j++) {
                path[j] = path[j+2];
                path[j+2] = "";
            }
            path_length = path_length - 2;
            i = i - 2;
        }
    }
}

// Configuración inicial del robot
void setup() {
    Serial.begin(9600);

    path_length = 0;
    for (int i = 0; i < 50; i++) {
        path[i] = '0';
    }
    display.setLayout11x4();
    buzzer.play(">g32>>c32");

    display.clear();
    display.gotoXY(0, 1);
    display.print(" PRESS B");
    display.gotoXY(0, 2);
    display.print(" TO CALIB");

    // Espera la pulsación del botón B para iniciar la calibración
    while (!buttonB.getSingleDebouncedPress());
    display.clear();
    calibrateSensors();
    showReadings();

    display.clear();
    display.gotoXY(0, 1);
    display.print(" PRESS B");
    display.gotoXY(0, 2);
    display.print(" TO START");
    finish = 0;
    y = 0;

    while (!buttonB.getSingleDebouncedPress());

    // Reproduce una melodía de inicio
    display.clear();
    buzzer.play("L16 cdegreg4");
    while (buzzer.isPlaying());
}

// Bucle principal del robot
void loop() {
    if (finish == 0) {
        follow_segment();
        motors.setSpeeds(40, 40);
        delay(85);
        readADC();
        delay(90);

        motors.setSpeeds(0, 0);
        select_turn();
    } else if (finish == 1) {
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
        for (int j = 0; j < path_length; j++) {
            display.print(path[j]);
            if (j > 8)
                display.gotoXY(0, 1);
        }

        while (!buttonB.getSingleDebouncedPress());

        simplify_path();
        path[path_length-1] = "";
        path_length = path_length - 1;
        display.clear();
        for (int j = 0; j < path_length; j++) {
            display.print(path[j]);
        }
        while (!buttonB.getSingleDebouncedPress());
    }

    while (finish == 2) {
        display.clear();
        if (path[0] == 'U') {
            turnAround();
            y++;
        }
        follow_segment();
        motors.setSpeeds(40, 40);
        delay(85);
        readADC();
        delay(90);

        motors.setSpeeds(0, 0);
        follow_pathricio();
    }
    if (finish == 3) {
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
