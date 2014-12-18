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

const char BLUETOOTH_DEVICE_NAME[] = "wearable";
const char BLUETOOTH_DEVICE_PIN[5] = "1234";

#define DEVICE_NUMBER 1

/* Pins */
#define LED_PIN 13

enum Device 
{
    LED
};

/* Device ID code for the protocol */
const char DEVICE_CODE[DEVICE_NUMBER][3] = 
{
    "LE"  /* Led */
};

Pstate SM_STATE[DEVICE_NUMBER + 1] = 
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

SM stateMachine(waitForCommandState);
int bluetoothCommandArgument;

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
    char command[64];
    sprintf(command, "AT+NAME%s\n\r", BLUETOOTH_DEVICE_NAME);
    Serial1.print(command);
    delay(1100);
    sprintf(command, "AT+PIN%s\n\r", BLUETOOTH_DEVICE_PIN);
    Serial1.print(command);

    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    EXEC(stateMachine);
}

/****************************************************************
 * Function Definitions                                         *
 ****************************************************************/

State waitForCommandState()
{
    char commandString[7];
    char* command = commandString;
    char commandCode[3];
    char receivedChar;    
    int  receivedCharIndex = 0;

    stateMachine.Set(waitForCommandState);

    /* Get command from serial port */
    while (Serial.available())
    {
        receivedChar = (char) Serial.read();
        if (receivedChar == '\n' || receivedChar == '\n' || receivedCharIndex > 5)
        {
            receivedCharIndex++;
            continue;
        }

        command[receivedCharIndex] = receivedChar;
        receivedCharIndex++;
    }

    (receivedCharIndex > 5) ? command[6] = '\0' : command[receivedChar] = '\0';

    if (strlen(command) == 0) return;

    /* Parse command */
    if (command[0] != '#') return;

    #ifdef DEBUG
        Serial.print("Debug: Received command: ");
        Serial.println(commandString);
    #endif
    
    command++;
    for (int i = 0; i < 2; i++) commandCode[i] = command[i];
    commandCode[2] = '\0';

    command += 2;
    bluetoothCommandArgument = atoi(command);
    
    for (int i = 0; i < DEVICE_NUMBER; i++)
    {
        if (strcmp(commandCode, DEVICE_CODE[i]) == 0)
        {
            stateMachine.Set(SM_STATE[i]);
            return;
        }
    }
    
}

State ledState()
{
    #ifdef DEBUG
        Serial.println("Debug: LED state");
    #endif
    if (bluetoothCommandArgument)
    {
        digitalWrite(LED_PIN, HIGH);
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
    }

    stateMachine.Set(waitForCommandState);
}
