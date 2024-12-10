/** 
 * ESP8266 WeMos code for WIFI Wheeler
 * 
 * REVISION HISTORY 
 * v101 - Date: 20240911
 *      - cleaned up code and added comments
 * v102 - Date: 20240911
 *      - added #, * commands
 *      - updated pin assignments (d0-tx)
 **/

# include <Arduino.h>
# include <ESP8266WiFi.h> //import for wifi functionality
# include <WebSocketsServer.h> //import for websocket

//- pin assignments

static const uint8_t d0   = 16;
static const uint8_t d1   = 5;
static const uint8_t d2   = 4;
static const uint8_t d3   = 0;
static const uint8_t d4   = 2;
static const uint8_t d5   = 14;
static const uint8_t d6   = 12;
static const uint8_t d7   = 13;
static const uint8_t d8   = 15;
static const uint8_t rX   = 3;
static const uint8_t tX   = 1;

# define MOTOR_A1A d0
# define MOTOR_A1B d1
# define MOTOR_B1A d2
# define MOTOR_B1B d3
# define LEDPIN d4
# define ECHO d6
# define TRIGGER d7
# define MAX_SPEED 255 //- set max speed of car.  Value between 0 and 255
# define EDWARD 1


/** WIFI SSID and Password that will be broadcast from your ESP8266 board.
 * If you change these values, make sure to change your WIFI connection settings on your phone/PC
 */
const char *ssid =  "My Barnabas Bot";      //- WIFI SSID (Network Name)
const char *pass =  "12345678";             //- WIFI password

WebSocketsServer webSocket = WebSocketsServer(81); //- Initialize WebSocket Server using Port 81

/** Mandatory setup function.  
 * Set up pins and WIFI stuff
 */
void setup() {
    pinMode(LEDPIN, OUTPUT); 
    pinMode(MOTOR_A1A, OUTPUT);
    pinMode(MOTOR_A1B, OUTPUT);
    pinMode(MOTOR_B1A, OUTPUT);
    pinMode(MOTOR_B1B, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(TRIGGER, OUTPUT);
    
    stopAllMotors(); //- make sure that all WIFI Wheeler motors are off by default
    
    Serial.begin(9600); //- Turn on serial monitor for debugging.  Set baud to 9600 bps
    Serial.println("Connecting to wifi");
    
    IPAddress apIP(192, 168, 0, 1);   //- static IP for your ESP8266 board.  
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); 
    WiFi.softAP(ssid, pass); //- turn on WIFI
    
    webSocket.begin(); //- Initialize WebSocket Handling
    webSocket.onEvent(webSocketEvent); //- Allow WebSocket to detect events
    
    Serial.println("Websocket is started");
}

/** Mandatory loop function.  Runs over and over.  Keeps the WebSocket running
 * 
 */
void loop() {
   webSocket.loop(); //- keep this line on loop method
}

//- Handles all events on the WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    String cmd = "";
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("Websocket is disconnected");
            stopAllMotors(); //-stop the motors if disconnected
            break;
        case WStype_CONNECTED:{
            Serial.println("Websocket is connected");
            Serial.println(webSocket.remoteIP(num).toString());
            webSocket.sendTXT(num, "connected");}
            break;
        case WStype_TEXT:
            cmd = "";
            //merging payload to single string
            for(int i = 0; i < length; i++) {
                cmd = cmd + (char) payload[i];
            } 
            handleText(cmd, num);
            break;
        case WStype_FRAGMENT_TEXT_START:
            break;
        case WStype_FRAGMENT_BIN_START:
            break;
        case WStype_BIN:
            hexdump(payload, length);
            break;
        default:
            break;
    }
}

/** DC Motor Driver Helper Function
 * 
 */

//- stop both motor A and motor B
void stopAllMotors() {
    analogWrite(MOTOR_A1A,0);
    analogWrite(MOTOR_A1B,0);
    
    analogWrite(MOTOR_B1A,0);
    analogWrite(MOTOR_B1B,0);
}


void moveForwardSpeed(int speed) {
    analogWrite(MOTOR_A1A,0);
    analogWrite(MOTOR_A1B,speed);
    
    analogWrite(MOTOR_B1A,speed);
    analogWrite(MOTOR_B1B,0);    
}

//- move WIFI Wheeler forward at half speed
void moveForward() {
    analogWrite(MOTOR_A1A,0);
    analogWrite(MOTOR_A1B,MAX_SPEED*.5);
    
    
    analogWrite(MOTOR_B1A,MAX_SPEED*.5);
    analogWrite(MOTOR_B1B,0);
}

//- move WIFI Wheeler backwards at half speed
void moveBackward() {
    analogWrite(MOTOR_A1A,MAX_SPEED*.5);
    analogWrite(MOTOR_A1B,0);
    analogWrite(MOTOR_B1A,0);
    analogWrite(MOTOR_B1B,MAX_SPEED*.5);
}

//- spin WIFI Wheeler left at half speed
void spinLeft() {
    analogWrite(MOTOR_A1A,0);
    analogWrite(MOTOR_A1B,MAX_SPEED*.5); 
    analogWrite(MOTOR_B1A,0);
    analogWrite(MOTOR_B1B,MAX_SPEED*.5);
}

//- spin WIFI Wheeler right at half speed
void spinRight() {
    analogWrite(MOTOR_A1A,MAX_SPEED*.5);
    analogWrite(MOTOR_A1B,0); 
    analogWrite(MOTOR_B1A,MAX_SPEED*.5);
    analogWrite(MOTOR_B1B,0);
}

//- have WIFI wheeler move forward and stops when it sees an object less than 5 cm away
void moveToObject() {
    int distance;
    distance = ultrasonic();
    
    while (distance > 5) {
        moveForward();
        distance = ultrasonic();  
        delay(100);
    }
    stopAllMotors();
}

//- use ultrasonic sensor to determine the distance of an object.  Returns distance in centimeters.
int ultrasonic() {
  
    long time;
    float distance;
    
    //-trigger a sound 
    // send out trigger signal
    digitalWrite(TRIGGER, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGGER, HIGH);
    delayMicroseconds(20);
    digitalWrite(TRIGGER, LOW);
    
    //- a sound has gone out!!
    //- wait for a sound to come back
    time = pulseIn(ECHO, HIGH);
    
    //- calculate the distance in centimeters
    distance = 0.01715 * time;
    
    return distance;

}

//- handle commands from your phone/computer
void handleText(String cmd, uint8_t num){
    
    Serial.println(cmd); //- Print command on serial monitor for debugging
  
    bool cmdFound = true; //- cmdFound default is true.  Set to false later if command is not found
  
    //- Onboard LED on-off
    if (cmd == "poweron") { 
        digitalWrite(LEDPIN, LOW);   
    }
    else if (cmd == "poweroff") {
        digitalWrite(LEDPIN, HIGH);   
    }
    //- Motor A Commands
    else if (cmd == "Servo1_0"){
        analogWrite(MOTOR_A1A,0);
        analogWrite(MOTOR_A1B,0);
    }
    else if (cmd == "Servo1_.25"){
        analogWrite(MOTOR_A1A,MAX_SPEED*.25);
        analogWrite(MOTOR_A1B,0);
    }
    else if (cmd == "Servo1_.5"){
        analogWrite(MOTOR_A1A,MAX_SPEED*.5);
        analogWrite(MOTOR_A1B,0);
    }
    else if (cmd == "Servo1_.75"){
        analogWrite(MOTOR_A1A,MAX_SPEED*.75);
        analogWrite(MOTOR_A1B,0);
    }
    else if (cmd == "Servo1_1"){
        analogWrite(MOTOR_A1A,MAX_SPEED*1.0);
        analogWrite(MOTOR_A1B,0);
    }
    else if (cmd == "Servo1_-.25"){
        analogWrite(MOTOR_A1A,0);
        analogWrite(MOTOR_A1B,MAX_SPEED*.25);
    }
    else if (cmd == "Servo1_-.5"){
        analogWrite(MOTOR_A1A,0);
        analogWrite(MOTOR_A1B,MAX_SPEED*.5);
    }
    else if (cmd == "Servo1_-.75"){
        analogWrite(MOTOR_A1A,0);
        analogWrite(MOTOR_A1B,MAX_SPEED*.75);
    }
    else if (cmd == "Servo1_-1"){
        analogWrite(MOTOR_A1A,0);
        analogWrite(MOTOR_A1B,MAX_SPEED*1);
    }
    //- Motor B Commands
    else if (cmd == "Servo2_0"){
        analogWrite(MOTOR_B1A,0);
        analogWrite(MOTOR_B1B,0);
    }
    else if (cmd == "Servo2_.25"){
        analogWrite(MOTOR_B1A,0);
        analogWrite(MOTOR_B1B,MAX_SPEED*.25);
    }
    else if (cmd == "Servo2_.5"){
        analogWrite(MOTOR_B1A,0);
        analogWrite(MOTOR_B1B,MAX_SPEED*.5);
    }
    else if (cmd == "Servo2_.75"){
        analogWrite(MOTOR_B1A,0);
        analogWrite(MOTOR_B1B,MAX_SPEED*.75);
    }
    else if (cmd == "Servo2_1"){
        analogWrite(MOTOR_B1A,0);
        analogWrite(MOTOR_B1B,MAX_SPEED*1);
    }
    else if (cmd == "Servo2_-.25"){
        analogWrite(MOTOR_B1A,MAX_SPEED*.25);
        analogWrite(MOTOR_B1B,0);
    }
    else if (cmd == "Servo2_-.5"){
        analogWrite(MOTOR_B1A,MAX_SPEED*.5);
        analogWrite(MOTOR_B1B,0);
    }
    else if (cmd == "Servo2_-.75"){
        analogWrite(MOTOR_B1A,MAX_SPEED*.75);
        analogWrite(MOTOR_B1B,0);
    }
    else if (cmd == "Servo2_-1"){
        analogWrite(MOTOR_B1A,MAX_SPEED*1);
        analogWrite(MOTOR_B1B,0);
    }    
    //- Reserved Control Panel Commands
    else if (cmd == "0") {
        //-code here
        stopAllMotors();
    }
    else if (cmd == "1") {
        //-code here
        moveBackward();
    }
    else if (cmd == "2") {
        //-code here
        moveForward();
    }
    else if (cmd == "3") {
        //-code here
        spinLeft();
    }
    else if (cmd == "4") {
        //-code here
        spinRight();
    }
    else if (cmd == "5") {
        //-code here
        moveToObject();
    }
    else if (cmd == "6") {
        //-code here
        //-sense distace of nearest object
        cmd = cmd + " = " + ultrasonic() + " cm";
    }
    else if (cmd == "7") {
        //-code here
        moveForward();
        delay(1000);
        stopAllMotors();
        moveBackward();
        delay(1000);
        stopAllMotors();
    }
    else if (cmd == "8") {
        //-code here
        digitalWrite(LEDPIN,LOW); //on
    }
    else if (cmd == "9") {
        //-code here
        digitalWrite(LEDPIN,HIGH); //off
    }
    else if (cmd == "*") {
        
        int counter;
        counter = 0;
        
        while ( counter < 5 ) {
            digitalWrite(LEDPIN,LOW); //on
            delay(500);
            digitalWrite(LEDPIN,HIGH); //off
            delay(500);
            counter = counter + 1;
        }
        
        //-code here
    }
    else if (cmd == "#") {
        //-code here
        
        int counter;
        counter = 255;
        
        while (counter > 0) {
            moveForwardSpeed(counter);
            counter = counter - 1;
            delay(10);
        }
        stopAllMotors();
    }
    else {
        //- this is an unknown command
        cmdFound = false;
    }
        
    //- build response.  Append ':success' if commands is found.  Append ':command not found' if it is an unknown command
    if( cmdFound ){
        cmd = cmd + ":success";
    }
    else {
        cmd = cmd + ":Command not found";
    }
    
    //- send response via webSocket
    webSocket.sendTXT(num, cmd);
}