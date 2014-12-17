/****************************************************************
 ***********************KIT WEARABLE - CP *********************** 
 ****************************************************************/

/****************************************************************
 * Constants and pins                                           *
 ****************************************************************/

const char BLUETOOTH_DEVICE_NAME[] = "wearable";
const char BLUETOOTH_DEVICE_PIN[5] = "1234";

/****************************************************************
 * Function Prototypes                                          *
 ****************************************************************/

/****************************************************************
 * Global variables                                             *
 ****************************************************************/

/****************************************************************
 * Arduino main functions                                       *
 ****************************************************************/

void setup()
{   
    Serial.begin(9600);
    
    /* Setup HC-06 bluetooth module */
    Serial1.begin(9600);  /* HC-06 default */
    char command[64];
    sprintf(command, "AT+NAME%s\n\r", BLUETOOTH_DEVICE_NAME);
    Serial1.print(command);
    delay(1100);
    sprintf(command, "AT+PIN%s\n\r", BLUETOOTH_DEVICE_PIN);
    Serial1.print(command);
}

void loop()
{
    while (Serial1.available())
    {
       Serial.write((char) Serial1.read()); 
    }

    while (Serial.available())
    {
        Serial1.write(Serial.read());
    }
    delay(100);
}

/****************************************************************
 * Function Definitions                                         *
 ****************************************************************/
