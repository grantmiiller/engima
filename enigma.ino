// Demo TFT Display and 4x4 Keypad for Black Pill STM32

#include "SPI.h"
#include <Adafruit_GFX_AS.h>    
#include <Adafruit_ILI9341_STM.h> 

#define NO_KEY 0
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

const int TOTAL_KEYS = ROWS * COLS;
char chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', '*', '#'};

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {PB12, PB13, PB14, PB15}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {PA8, PB5, PB6, PB7}; //connect to the column pinouts of the keypad

#define TFT_CS         PA1                 
#define TFT_DC         PA3               
#define TFT_RST        PA2 

// Just a pin with an LED to show true or false values
#define DEBUG_PIN      PB11

// The pin to listen for enter state
#define ENTER_PIN PB8

// definitions of states
// State used to short circuit, mostly for debugging
#define STATE_NO_OP 0
// Getting if we are Decrypting or decrypting
#define STATE_GET_EN_DE 1
#define STATE_GET_KEY 2
#define STATE_GET_MESSAGE 3
#define STATE_PRINT_OUTPUT 4

// State if we are encrypting or decrypting
#define ENCRYPT_STATE 1
#define DECRYPT_STATE 2


// Boolean to show if user is trying to enter a value
int ENTER_FLAG = 0;
// How long until we allow a new enter. This is to prevent a button press and then immediate unpress due to a glitch
int ENTER_DELTA = 300;
// Last time enter state was flipped
int LAST_ENTER_TIME = 0;

int APP_STATE = STATE_GET_EN_DE;
int ENC_DEC_STATE = 0;
int WAITING_FOR_INPUT_STATE = 0;

const int KEY_MAX_SIZE = 10;
char ENCRYPTION_KEY[KEY_MAX_SIZE];
int KEY_INDEX = 0;

const int MESSAGE_MAX_SIZE = 100;
char MESSAGE[MESSAGE_MAX_SIZE];
int MESSAGE_INDEX = 0;
 
Adafruit_ILI9341_STM tft = Adafruit_ILI9341_STM(TFT_CS, TFT_DC, TFT_RST); 

void setup() {
  tft.begin();
  tft.setRotation(3);
  clear_screen();
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(2);
  keypad_setup();
  get_encrypt_decrypt_state();
  pinMode(DEBUG_PIN, OUTPUT);

  // Make sure enter pin is set to low
  pinMode(ENTER_PIN, INPUT);
  digitalWrite(ENTER_PIN, LOW);
}

void clear_screen() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
}

void get_encrypt_decrypt_state() {
  clear_screen();
  tft.println("Choose Option:");
  tft.println("1) Encrypt");
  tft.println("2) Decrypt");
}

char get_key_press() {
  char key = getKeyPad();
  return key;
}

void print_key_press() {
  char key = get_key_press();

  if (key != NO_KEY) {
    tft.print(key);
    delay(500);
  }
}

void read_enter_state() {
  int current_time = millis();
  if (current_time - LAST_ENTER_TIME > ENTER_DELTA) {
    if(digitalRead(ENTER_PIN) == HIGH) {
      ENTER_FLAG = 1;
    } else {
      ENTER_FLAG = 0;
    }
    LAST_ENTER_TIME = current_time;
  }
}

void toggle_enter_state(int toggle) {
  int current_time = millis();
  ENTER_FLAG = toggle;
  LAST_ENTER_TIME = current_time;
}

void get_de_en_state_logic() {
  char key = get_key_press();
  if (key == NO_KEY) { return; }
  if (key != '1' && key != '2') {
    clear_screen();
    tft.println("Invalid Option.");
    tft.print(key);
    delay(1000);
    get_encrypt_decrypt_state();
  } else {
    clear_screen();
    if (key == '1') {
      ENC_DEC_STATE = ENCRYPT_STATE;
      tft.println("You choose Encrypt");
    }
    if (key == '2') {
      ENC_DEC_STATE = DECRYPT_STATE;
      tft.println("You choose Decrypt");
    }
    APP_STATE = STATE_GET_KEY;
    WAITING_FOR_INPUT_STATE = 0;
    delay(1000);
  }
}

void get_key_logic() {
  if(!WAITING_FOR_INPUT_STATE) {
    clear_screen();
    tft.println("Enter key:");
    WAITING_FOR_INPUT_STATE = 1;  
  } else {
    char key = get_key_press();

    // Switch to next state
    if (KEY_INDEX + 1 >= KEY_MAX_SIZE || (ENTER_FLAG && KEY_INDEX >= 1)) {
      APP_STATE = STATE_GET_MESSAGE;
      WAITING_FOR_INPUT_STATE = 0;
      toggle_enter_state(0);
      clear_screen();
      tft.println("The key you entered is: \n");
      for (int i = 0; i < KEY_INDEX; i++) {
        tft.print(ENCRYPTION_KEY[i]);
      }
      delay(300);
      return;
    }
    
    if (key == NO_KEY) { return; }
    
    ENCRYPTION_KEY[KEY_INDEX] = key;

    tft.print(key);
    
    KEY_INDEX++;
    
    delay(500);
  }

}

void get_message_logic() {
  if(!WAITING_FOR_INPUT_STATE) {
    clear_screen();
    tft.println("Enter message:");
    WAITING_FOR_INPUT_STATE = 1;  
  } else {
    char key = get_key_press();

    // Switch to next state
    if (MESSAGE_INDEX + 1 >= MESSAGE_MAX_SIZE || (ENTER_FLAG && MESSAGE_INDEX >= 1)) {
      APP_STATE = STATE_PRINT_OUTPUT;
      WAITING_FOR_INPUT_STATE = 0;
      toggle_enter_state(0);
      clear_screen();
      tft.println("The message you entered is: \n");
      for (int i = 0; i < MESSAGE_INDEX; i++) {
        tft.print(MESSAGE[i]);
      }
      delay(300);
      clear_screen();
      return;
    }
    
    if (key == NO_KEY) { return; }
    
    MESSAGE[MESSAGE_INDEX] = key;

    tft.print(key);
    
    MESSAGE_INDEX++;
    
    delay(500);
  }
}

char* generate_rotors(int seed) {
  randomSeed(seed);
  char newArray[TOTAL_KEYS] = {0};
  for (int i= 0; i< TOTAL_KEYS; i++) 
   {
     int pos = random(TOTAL_KEYS);
     int t = chars[i];   
     newArray[i] = chars[pos];
     newArray[pos] = t;
   }
   return newArray;
}

void print_message() {
  char* rotor_1[TOTAL_KEYS] = {0};
  rotor_1[TOTAL_KEYS] = generate_rotors(1);
   
  for (int i= 0; i< TOTAL_KEYS; i++) {
    tft.print(rotor_1[i]);
  }  
}

void loop(void) {
  if (true) {
    
  } else {
    return;
  }
  read_enter_state();
  if (ENTER_FLAG == 1) {
    digitalWrite(DEBUG_PIN, HIGH);
  } else {
    digitalWrite(DEBUG_PIN, LOW);
  }
  // Logic for deciding if encrypting or decrypting
  if (APP_STATE == STATE_GET_EN_DE) {
    get_de_en_state_logic();
  }
  if (APP_STATE == STATE_GET_KEY) {
    get_key_logic();
  }
  if (APP_STATE == STATE_GET_MESSAGE) {
    get_message_logic();
  }
  if(APP_STATE == STATE_PRINT_OUTPUT) {
    print_message();
  }
}

void keypad_setup() {
  for(int i=0; i<ROWS; i++) {
    pinMode(rowPins[i], INPUT);
  }
  for (int i=0; i<COLS; i++) {
    pinMode(colPins[i], INPUT);
  }  
}

char getKeyPad() {
    char keyFound = NO_KEY;
    // iterate the columns
    for (int colIndex=0; colIndex < COLS; colIndex++) {
        // col: set to output to low
        byte curCol = colPins[colIndex];
        pinMode(curCol, OUTPUT);
        digitalWrite(curCol, LOW);
 
        // row: interate through the rows
        for (int rowIndex=0; rowIndex < ROWS; rowIndex++) {
            byte rowCol = rowPins[rowIndex];
            pinMode(rowCol, INPUT_PULLUP);
            if ( ! digitalRead(rowCol) )
               keyFound = keys[rowIndex][colIndex];
            pinMode(rowCol, INPUT);
        }
        // disable the column
        pinMode(curCol, INPUT);
    }
    return keyFound;
}

