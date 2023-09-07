#include "Arduino.h"
#include "Keypad_MCP23008.h"

#define I2C0_SDA_PIN 0 // change depending on which pin is used
#define I2C0_SCL_PIN 1 // change depending on which pin is used
#define MCP23008_I2C_ADDRESS 0x20 // default value if no jumpers, can range from 0x20 to 0x28

#define NUM_ROWS 3 // can be up to 4 for a 16 button keypad
#define NUM_COLS 4

// may need to change depending on keypad pin layout
const uint8_t rowPins[] = {0, 1, 2};
const uint8_t colPins[] = {3, 4, 5, 6};

// can use I2C0 or I2C1 (each have different pin options, check Pico pinout)
// If I2C0, use Wire. If I2C1, use Wire1
Keypad_MCP23008 keypad(NUM_ROWS, NUM_COLS, rowPins, colPins, MCP23008_I2C_ADDRESS, &Wire);

void setup(){
    Serial.begin(115200);
    Wire.setSDA(I2C0_SDA_PIN);
    Wire.setSCL(I2C0_SCL_PIN);
    keypad.begin();
    keypad.setReadDelay(20); // optional, change time between updates. Defaults to 10ms
    keypad.setDebounceCount(3); // optional. Increase if experiencing weird behavior. Defaults to 1
    //keypad.setDebugMode(true); // turns on debugging, off by default, needs to be off when communicated with Unity
}

void loop(){
    if(keypad.update()){
        Serial.println(keypad.read()); // keypad.read() will always return the current state, returns values "normally"
                                       // it will also reset the "updated" flag
                                       // in binary, will return "1" as 00000001, "2" as 00000010, 3 as 00000011, etc.
                                       // will return 255 if error. Should automatically reset if error

        // keypad.printBinary(keypad.readBoolean(), NUM_ROWS * NUM_COLS); // printBinary only works if debugging is on
        // readBoolean will return "1" as 000000000001, "2" as 000000000010, etc.
        // will return 65535 if error
        // readBoolean not recommended for use. Has to return an int instead of byte
    }
    
}