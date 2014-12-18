/****************************************************************
 ***********************KIT WEARABLE - CP *********************** 
 ****************************************************************/

/****************************************************************
 * Libraries                                                    *
 ****************************************************************/

#include <SM.h>

/****************************************************************
 * Constants and pins                                           *
 ****************************************************************/

#define DEBUG
#define DEBUG_DEVICE_SM
#define DEBUG_PROTOCOL_SM

const char BLUETOOTH_DEVICE_NAME[] = "wearable";
const char BLUETOOTH_DEVICE_PIN[5] = "1234";

#define DEVICE_NUMBER 1

/* Pins */
#define LED_PIN 13

enum Device 
{
    LED
};

/* Device ID code for the bluetooth protocol */
const char DEVICE_CODE[DEVICE_NUMBER][3] = 
{
    "LE"  /* Led */
};
#define PROTOCOL_COMMAND_LENGHT 5
#define PROTOCOL_DEVICE_CODE_LENGTH 2

Pstate DEVICE_STATE[DEVICE_NUMBER + 1] = 
{
    ledState,
    waitForCommandState  /* Must always be last */
};

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

char protocolCommandString[PROTOCOL_COMMAND_LENGHT + 1];
char* protocolCommand;
int protocolCommandArgument;

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

    pinMode(LED_PIN, OUTPUT);
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
    protocolCommandArgument = atoi(protocolCommand);    

    #ifdef DEBUG
        Serial.print("Debug: Parsed Command Argument \"");
        Serial.print(protocolCommandArgument);
        Serial.println("\"");
    #endif

    readyToReceiveProtocolCommand = true;
    return;    
}

State ledState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: LED state");
    #endif

    if (protocolCommandArgument)
    {
        digitalWrite(LED_PIN, HIGH);
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
    }

    deviceStateMachine.Set(waitForCommandState);
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
