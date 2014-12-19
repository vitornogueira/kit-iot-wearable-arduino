/****************************************************************
 ***********************KIT WEARABLE - CP *********************** 
 ****************************************************************/

/****************************************************************
 * Libraries                                                    *
 ****************************************************************/

#include <SM.h>
#include <Wire.h>
#include <MMA7660.h>

/****************************************************************
 * Constants and pins                                           *
 ****************************************************************/

#define DEBUG
#define DEBUG_DEVICE_SM
#define DEBUG_PROTOCOL_SM

const char BLUETOOTH_DEVICE_NAME[] = "wearable";
const char BLUETOOTH_DEVICE_PIN[5] = "1234";

#define DEVICE_NUMBER 5

/* Pins */
#define RED_RGB_LED_PIN              5
#define GREEN_RGB_LED_PIN           13
#define BLUE_RGB_LED_PIN             6
#define ACCELEROMETER_INTERRUPT_PIN  7
#define LIGHT_SENSOR_PIN            12

enum Device 
{
    ACCELEROMETER,
    RED_RGB_LED,
    GREEN_RGB_LED,
    BLUE_RGB_LED,
    LIGHT_SENSOR
};

/* Device ID code for the bluetooth protocol */
#define PROTOCOL_REQUEST_LENGTH     5
#define PROTOCOL_DEVICE_CODE_LENGTH 2
#define PROTOCOL_RESPONSE_LENGTH    7
const char DEVICE_CODE[DEVICE_NUMBER][PROTOCOL_DEVICE_CODE_LENGTH + 1] = 
{
    "AC", /* Accelerometer */
    "LR", /* Red RGB Led */
    "LG", /* Green RGB Led */
    "LB", /* Blue RGB Led */
    "LI"  /* Light sensor */
};

Pstate DEVICE_STATE[DEVICE_NUMBER + 2] = 
{
    accelerometerState,
    redRgbLedState,
    greenRgbLedState,
    blueRgbLedState,
    lightSensorState,
    waitForCommandState,  /* Must always come after device states */
    sendValueState
};

/* Accelerometer axis */
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

/* RGB LED boundary values */
#define MIN_RGB_VALUE   0
#define MAX_RGB_VALUE 255

/****************************************************************
 * Function Prototypes                                          *
 ****************************************************************/

/****************************************************************
 * Global variables                                             *
 ****************************************************************/

SM deviceStateMachine(waitForCommandState);
SM protocolStateMachine(sleepState);

bool protocolCommandIsReady = false;
bool readyToReceiveProtocolCommand = true;

char protocolCommandString[PROTOCOL_REQUEST_LENGTH + 1];
char* protocolCommand;
int protocolCommandArgument;
char protocolResponseValue[PROTOCOL_RESPONSE_LENGTH + 1];

/****************************************************************
 * Arduino main functions                                       *
 ****************************************************************/

void setup()
{
    #ifdef DEBUG
        Serial.begin(9600);
    #endif
    
    /* Setup HC-06 bluetooth module */
    Serial1.begin(9600);  /* HC-06 default */
    char atCommand[64];
    sprintf(atCommand, "AT+NAME%s\n\r", BLUETOOTH_DEVICE_NAME);
    Serial1.print(atCommand);
    delay(1100);
    sprintf(atCommand, "AT+PIN%s\n\r", BLUETOOTH_DEVICE_PIN);
    Serial1.print(atCommand);

    /* Setup accelerometer */
    MMA7660.init();

    pinMode(RED_RGB_LED_PIN, OUTPUT);
    pinMode(GREEN_RGB_LED_PIN, OUTPUT);
    pinMode(BLUE_RGB_LED_PIN, OUTPUT);

    pinMode(LIGHT_SENSOR_PIN, INPUT);
}

void loop()
{
    EXEC(deviceStateMachine);
    EXEC(protocolStateMachine);
}

/****************************************************************
 * Device state machine functions                               *
 ****************************************************************/

State waitForCommandState()
{
    char deviceCode[PROTOCOL_DEVICE_CODE_LENGTH + 1];
    
    deviceStateMachine.Set(waitForCommandState);

    if (!protocolCommandIsReady) return;

    /* Parse device code */
    for (int i = 0; i < PROTOCOL_DEVICE_CODE_LENGTH; i++) deviceCode[i] = protocolCommand[i];
    deviceCode[PROTOCOL_DEVICE_CODE_LENGTH] = '\0';

    #ifdef DEBUG
        Serial.print("Debug: Parsed Command Device Code \"");
        Serial.print(deviceCode);
        Serial.println("\"");
    #endif

    for (int i = 0; i < DEVICE_NUMBER; i++)
    {
        if (strcmp(deviceCode, DEVICE_CODE[i]) == 0)
        {
            deviceStateMachine.Set(DEVICE_STATE[i]);
        }
    }

    /* Parse command argument */
    protocolCommand += PROTOCOL_DEVICE_CODE_LENGTH;

    for (int i = 0; i++; i < strlen(protocolCommand))
    {
        if (protocolCommand[i] < '0' || protocolCommand[i] > '9')
        {
            deviceStateMachine.Set(waitForCommandState);
            readyToReceiveProtocolCommand = true;
            return;
        }
    }
    protocolCommandArgument = atoi(protocolCommand);    

    #ifdef DEBUG
        Serial.print("Debug: Parsed Command Argument \"");
        Serial.print(protocolCommandArgument);
        Serial.println("\"");
    #endif

    readyToReceiveProtocolCommand = true;
    return;    
}

State sendValueState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Send Value State");
    #endif
    #ifdef DEBUG
        Serial.print("Debug: Sending value ");
        Serial.print(protocolResponseValue);
    #endif

    Serial1.println(protocolResponseValue);
    deviceStateMachine.Set(waitForCommandState);
}

State accelerometerState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Accelerometer State");
    #endif

    int x, y, z;

    delay(100); /* Time for new values to come */

    MMA7660.getValues(&x, &y, &z);

    switch(protocolCommandArgument)
    {
        case X_AXIS:
            sprintf(protocolResponseValue, "#%4d\n\r", x);
            break;

        case Y_AXIS:
            sprintf(protocolResponseValue, "#%4d\n\r", y);
            break;

        case Z_AXIS:
            sprintf(protocolResponseValue, "#%4d\n\r", z);
            break;

        default:
            deviceStateMachine.Set(waitForCommandState);
            return;
    }

    deviceStateMachine.Set(sendValueState);
}

State redRgbLedState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Red RGB LED State");
    #endif

    if ((protocolCommandArgument >= MIN_RGB_VALUE) && (protocolCommandArgument <= MAX_RGB_VALUE))
    {
        analogWrite(RED_RGB_LED_PIN, protocolCommandArgument);
    }

    deviceStateMachine.Set(waitForCommandState);
}

State greenRgbLedState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Green RGB LED State");
    #endif
    
    if ((protocolCommandArgument >= MIN_RGB_VALUE) && (protocolCommandArgument <= MAX_RGB_VALUE))
    {
        analogWrite(GREEN_RGB_LED_PIN, protocolCommandArgument);
    }
    
    deviceStateMachine.Set(waitForCommandState);
}

State blueRgbLedState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Blue RGB LED State");
    #endif
    
    if ((protocolCommandArgument >= MIN_RGB_VALUE) && (protocolCommandArgument <= MAX_RGB_VALUE))
    {
        analogWrite(BLUE_RGB_LED_PIN, protocolCommandArgument);
    }
    
    deviceStateMachine.Set(waitForCommandState);
}

State lightSensorState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Light Sensor State");
    #endif

    int lightSensorValue = analogRead(LIGHT_SENSOR_PIN);
    sprintf(protocolResponseValue, "#%4d\n\r", lightSensorValue);

    deviceStateMachine.Set(sendValueState);
}

/****************************************************************
 * Protocol state machine functions                             *
 ****************************************************************/

State sleepState()
{
    if (readyToReceiveProtocolCommand)
    {
        protocolStateMachine.Set(waitForStartState);
        protocolCommandIsReady = false;
    }
    else
    {
        protocolStateMachine.Set(sleepState);
    }
}

State waitForStartState()
{
    if (!Serial1.available())
    {
        protocolStateMachine.Set(waitForStartState);
    }
    else
    {
        char receivedChar = Serial1.read();
        if (receivedChar == '#')
        {
            /* Start of protocol command detetcted */
            #ifdef DEBUG_PROTOCOL_SM
                Serial.println("Debug Protocol State Machine: Wait For Start State => '#'");
            #endif

            strcpy(protocolCommandString, "");
            protocolCommand = protocolCommandString;
            protocolStateMachine.Set(receiveCharState);
        }
        else
        {
            protocolStateMachine.Set(waitForStartState);
        }
    }
}

State receiveCharState()
{
    if (!Serial1.available())
    {
        protocolStateMachine.Set(receiveCharState);
    }
    else
    {
        char receivedChar = Serial1.read();

        #ifdef DEBUG_PROTOCOL_SM
            Serial.print("Debug Protocol State Machine: Receive Char State => '");
            Serial.print(receivedChar);
            Serial.println("'");
        #endif

        if (receivedChar == '\n' || receivedChar == '\r' || strlen(protocolCommand) > 5)
        {
            /* End of protocol command detected */
            *protocolCommand = '\0';
            protocolCommandIsReady = true;
            readyToReceiveProtocolCommand = false;
            protocolCommand = protocolCommandString;
            protocolStateMachine.Set(sleepState);
            
            #ifdef DEBUG
                Serial.print("Debug: Received command \"#");
                Serial.print(protocolCommand);
                Serial.println("\"");
            #endif
        }
        else            
        {
            /* Keep receiving chars unitl the end is detected */
            *protocolCommand = receivedChar;
            *(protocolCommand + 1) = '\0';
            protocolCommand++;
            protocolStateMachine.Set(receiveCharState);
        }
    }
}
