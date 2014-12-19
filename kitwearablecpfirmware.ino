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

#define DEVICE_NUMBER     7
#define NUMBER_OF_BUTTONS 2

/* Pins */
#define RED_RGB_LED_PIN              5
#define GREEN_RGB_LED_PIN           13
#define BLUE_RGB_LED_PIN             6
#define ACCELEROMETER_INTERRUPT_PIN  7
#define LIGHT_SENSOR_PIN            12
#define BUZZER_PIN                  11
#define BUTTON1_PIN                  4
#define BUTTON2_PIN                 A1

enum Device 
{
    ACCELEROMETER,
    RED_RGB_LED,
    GREEN_RGB_LED,
    BLUE_RGB_LED,
    LIGHT_SENSOR,
    BUZZER,
    PLAY_MELODY
};

/* Device ID code for the bluetooth protocol */
#define PROTOCOL_REQUEST_LENGTH     6
#define PROTOCOL_DEVICE_CODE_LENGTH 2
#define PROTOCOL_RESPONSE_LENGTH    7
const char DEVICE_CODE[DEVICE_NUMBER][PROTOCOL_DEVICE_CODE_LENGTH + 1] = 
{
    "AC", /* Accelerometer */
    "LR", /* Red RGB Led */
    "LG", /* Green RGB Led */
    "LB", /* Blue RGB Led */
    "LI", /* Light sensor */
    "BZ", /* Buzzer */
    "PM"  /* Play Mario Melody */
};

Pstate DEVICE_STATE[DEVICE_NUMBER + 3] = 
{
    accelerometerState,
    redRgbLedState,
    greenRgbLedState,
    blueRgbLedState,
    lightSensorState,
    buzzerState,
    playMelodyState,
    waitForCommandState,  /* Must always come after device states */
    sendValueState,
    processButtonState
};

const char BUTTON1_CODE[] = "B1"; /* Button 1 */
const char BUTTON2_CODE[] = "B2"; /* Button 2 */

/* Accelerometer axis */
#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

/* RGB LED boundary values */
#define MIN_RGB_VALUE   0
#define MAX_RGB_VALUE 255

#define MELODY_MAX_SIZE      100
/* Melody codes */
#define MARIO_THEME_SONG    6789
#define CHRISTMAS_SONG_CODE 1234

/****************************************************************
 * Melodies                                                     *
 ****************************************************************/

int MARIO_THEME_SONG_MELODY[] = 
{
      2637, 2637, 0, 2637,
      0, 2093, 2637, 0,
      3136, 0, 0,  0,
      1568, 0, 0, 0,
             
      2093, 0, 0, 1568,
      0, 0, 1319, 0,
      0, 1760, 0, 1976,
      0, 1865, 1760, 0,
                     
      1568, 2637, 3136,
      3520, 0, 2794, 3136,
      0, 2637, 0, 2093,
      2349, 1976, 0, 0,
                            
      2093, 0, 0, 1568,
      0, 0, 1319, 0,
      0, 1760, 0, 1976,
      0, 1865, 1760, 0,
                                       
      1568, 2637, 3136,
      3520, 0, 2794, 3136,
      0, 2637, 0, 2093,
      2349, 1976, 0, 0
};
int MARIO_THEME_SONG_SIZE = 78;
int MARIO_THEME_SONG_TEMPO[] = 
{
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12,
             
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12,
                
      9, 9, 9,
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12,
             
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12,
         
      9, 9, 9,
      12, 12, 12, 12,
      12, 12, 12, 12,
      12, 12, 12, 12
};

int CHRISTMAS_THEME_SONG_MELODY[] =
{
      147, 247, 220, 196, 
      147, 147, 147, 147, 
      247, 220, 196, 165, 
      0, 165, 262, 247, 
      220, 185, 0, 294, 
      294, 262, 220, 247
};
int CHRISTMAS_THEME_SONG_SIZE = 24;
int CHRISTMAS_THEME_SONG_TEMPO[] =
{
      4, 4, 4, 4, 
      3, 8, 8, 4, 
      4, 4, 4, 3, 
      2, 4, 4, 4, 
      4, 3, 2, 4, 
      4, 4, 4, 1
};

/****************************************************************
 * Function Prototypes                                          *
 ****************************************************************/

void playMelody(int melody[], int tempo[], int melodySize);

/****************************************************************
 * Global variables                                             *
 ****************************************************************/

SM deviceStateMachine(waitForCommandState);
SM protocolStateMachine(sleepState);

bool protocolCommandIsReady = false;
bool readyToReceiveProtocolCommand = true;

char protocolCommandString[2 * PROTOCOL_REQUEST_LENGTH];
char* protocolCommand;
int protocolCommandArgument;
char protocolResponseValue[PROTOCOL_RESPONSE_LENGTH + 1];

int button1 = LOW;
int button2 = LOW;

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

    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
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
    
    deviceStateMachine.Set(processButtonState);

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
   
    /* Search for null argument or invalid characters */
    bool validArgument = true;
    for (int i = 0; i < strlen(protocolCommand); i++)
    {
        if (protocolCommand[i] < '0' || protocolCommand[i] > '9')
        {
            validArgument = false;
        }
    }
    if (strlen(protocolCommand) == 0) validArgument = false;

    if (!validArgument)
    {
        deviceStateMachine.Set(processButtonState);
        readyToReceiveProtocolCommand = true;
        return;
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

State processButtonState()
{
    if (button1 != digitalRead(BUTTON1_PIN))
    {
        button1 = digitalRead(BUTTON1_PIN);
        sprintf(protocolResponseValue, "#%s%2d\n\r", BUTTON1_CODE, !button1);
        deviceStateMachine.Set(sendValueState);
    }
    else if (button2 != digitalRead(BUTTON2_PIN))
    {
        button2 = digitalRead(BUTTON2_PIN);
        sprintf(protocolResponseValue, "#%s%2d\n\r", BUTTON2_CODE, !button2);
        deviceStateMachine.Set(sendValueState);
    }
    else
    {
        deviceStateMachine.Set(waitForCommandState);
    }
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

State buzzerState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Buzzer State");
    #endif
   
    if (protocolCommandArgument == 0) noTone(BUZZER_PIN);
    else tone(BUZZER_PIN, protocolCommandArgument);

    deviceStateMachine.Set(waitForCommandState);
}

State playMelodyState()
{
    #ifdef DEBUG_DEVICE_SM
        Serial.println("Debug Device State Machine: Play Melody State");
    #endif

    deviceStateMachine.Set(waitForCommandState);

    if (protocolCommandArgument == MARIO_THEME_SONG)
    {
        playMelody(MARIO_THEME_SONG_MELODY, MARIO_THEME_SONG_TEMPO, MARIO_THEME_SONG_SIZE);
    }
    else if (protocolCommandArgument == CHRISTMAS_SONG_CODE)
    {
        playMelody(CHRISTMAS_THEME_SONG_MELODY, CHRISTMAS_THEME_SONG_TEMPO, CHRISTMAS_THEME_SONG_SIZE);
        playMelody(CHRISTMAS_THEME_SONG_MELODY, CHRISTMAS_THEME_SONG_TEMPO, CHRISTMAS_THEME_SONG_SIZE);
    }
    else return;
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

            /* Reset protocol command string */
            sprintf(protocolCommandString, "");

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
            Serial.print("Debug Protocol State Machine: Current length of protocol command string is ");
            Serial.println(strlen(protocolCommandString));
        #endif

        /* Received command string is too long */
        if (strlen(protocolCommandString) > PROTOCOL_REQUEST_LENGTH)
        {
            protocolStateMachine.Set(sleepState);
            
            #ifdef DEBUG
                Serial.println("Debug: Failed to receive command (command string is too long)");
            #endif
            return;
        }

        if (receivedChar == '\n' || receivedChar == '\r')
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

/****************************************************************
 * Function Implementations                                     *
 ****************************************************************/

void playMelody(int melody[], int tempo[], int melodySize)
{
    for (int thisNote = 0; thisNote < melodySize; thisNote++) 
    {
         
        /* To calculate the note duration, take one second divided by the note type
           e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc. */
        int noteDuration = 1000 / tempo[thisNote];
        
        tone(BUZZER_PIN, melody[thisNote], noteDuration);
         
        /* To distinguish the notes, set a minimum time between them
           The note's duration + 30% seems to work well */
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
         
        /* Stop the tone playing */
        noTone(BUZZER_PIN);
    }
}
