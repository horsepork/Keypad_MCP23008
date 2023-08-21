#include "Adafruit_MCP23X08.h"

class Keypad_MCP23008{
    public:
        Keypad_MCP23008(uint8_t _numRows, uint8_t _numCols, const uint8_t *_rowPins, const uint8_t *_colPins, uint8_t _address, TwoWire *_wire){
            numRows = _numRows;
            numCols = _numCols;
            rowPins = _rowPins;
            colPins = _colPins;
            address = _address;
            wire = _wire;
            mcp = new Adafruit_MCP23X08(); // necessary?
        }
    private:
        uint8_t address;
        uint8_t numRows;
        uint8_t numCols;
        const uint8_t *rowPins;
        const uint8_t *colPins;
        TwoWire* wire;
        Adafruit_MCP23X08* mcp;
        uint8_t readDelay = 10;
        uint8_t debounceCount = 1;
        uint32_t timer;
        uint8_t GPIO_State = 0;
        bool resetNeeded = false;
        uint16_t prevState = 0;
        uint16_t outputState = 0;
        uint16_t debouncedBooleanOutputState = 0;
        uint8_t debouncedOutputState = 0;
        const uint16_t errorVal = 65535; // max val of uint16_t, "-1"
        bool debug = false;
        bool updated = false;

    
    public:
        void begin(){
            wire->setClock(400000);
            mcp->begin_I2C(address, wire);
            for(int i = 0; i < numRows; i++){
                mcp->pinMode(i, INPUT_PULLUP);
            }
            for(int i = numRows; i < numRows + numCols; i++){
                mcp->pinMode(i, OUTPUT);
                mcp->digitalWrite(i, HIGH);
            }
            GPIO_State = mcp->readGPIO();
            timer = millis();
        }

        void setReadDelay(uint8_t d){
            readDelay = d;
            if(!debug) return;
            Serial.print("read delay -- ");
            Serial.println(readDelay);
        }

        void setDebounceCount(uint8_t d){
            debounceCount = d;
            if(!debug) return;
            Serial.print("debounce time -- ");
            Serial.println(debounceCount);
        }

        void update(){
            if(millis() - timer < readDelay){
                return;
            }
            updateDebouncedOutputState();
            timer = millis();
            if(resetNeeded){
                if(!keypadReset()){
                    return;
                }
            }
            unsigned int newState = getNewState();
            int continuousBits = 0;
            if(newState == 0){
                outputState = 0;
                prevState = 0;
                return;
            }
            for(int i = 0; i < numRows * numCols; i++){
                if(bitRead(newState, i)){
                    if(++continuousBits > numCols){
                        resetNeeded = true;
                        outputState = errorVal;
                        return;
                    }
                }
                else{
                    continuousBits = 0;
                }
            }
            if(prevState != newState){
                unsigned int newBits = ~prevState & newState;
                
                int numNewBits = numBits(newBits);
                if(numNewBits == 1){
                    outputState = newBits;
                    
                }
                else if(numNewBits == 0){
                    outputState = 0;
                }
                prevState = newState;
            }
        }

        uint8_t read(bool print = false){
            if(print && debug){
                Serial.print("Keypad output --");
                Serial.println(debouncedOutputState);
            }
            updated = false;
            return debouncedOutputState;
        }

        uint16_t readBoolean(bool print = false){
            if(print && debug){
                Serial.print("Keypad output, boolean mode -- ");
                printBinary(debouncedBooleanOutputState, numRows * numCols);
            }
            updated = false;
            return debouncedBooleanOutputState;
        }

        void printBinary(int num, int digits = 8){
            if(!debug) return;
            if(num == 0){
                for(int i = 0; i < digits; i++){
                    Serial.print('0');
                }
                Serial.println();
                return;
            }
            int _num = num;
            int counter = 0;
            while(_num > 0){
                _num >>= 1;
                counter++;
            }
            if(counter < digits){
                for(int i = 0; i < digits - counter; i++){
                    Serial.print('0');
                }
            }
            Serial.println(num, BIN);
        }

        void setDebugMode(bool d){
            debug = d;
            Serial.print("Keypad debugging ");
            debug ? Serial.println("on") : Serial.println("off");
        }

        bool isConnected() {
            wire->beginTransmission(address);
            return wire->endTransmission() == 0;
        }

        bool isUpdated(){
            return updated;
        }


        
    private:
        void switchCol(int col){
            for(int i = 0; i < numCols; i++){
                if(i == col){
                    bitClear(GPIO_State, colPins[i]);
                }else{
                    bitSet(GPIO_State, colPins[i]);
                }
            }
            mcp->writeGPIO(GPIO_State);
        }

        int power(int base, int exp){
            int val = 1;
            for(int i = 0; i < exp; i++){
                val *= base;
            }
            return val;
        }
        bool keypadReset(){
            if(isConnected()){ // why? possible endless loop?
                // return false;
            }
            writeReg(0x06, 0xFF);
            for(int i = 0; i < numCols; i++){
                mcp->pinMode(i, INPUT_PULLUP);
            }
            for(int i = numRows; i < numRows + numCols; i++){
                mcp->pinMode(i, OUTPUT);
                mcp->digitalWrite(i, HIGH);
            }
            resetNeeded = false;
            if(debug){
                Serial.print("Keypad successfully reset");
            }
            return true;
        }

        bool writeReg(uint8_t reg, uint8_t value){
            wire->beginTransmission(address);
            wire->write(reg);
            wire->write(value);
            return wire->endTransmission() == 0;
        }

        int numBits(int num){
            if(num < 0){
                return 0;
            }
            int counter = 0;
            while(num > 0){
                counter += bitRead(num, 0);
                num >>= 1;
            }
            return counter;
        }

        int getNewState(){
            int _newState = 0;
            for(int c = 0; c < numCols; c++){
                switchCol(c);
                GPIO_State = mcp->readGPIO();
                for(int r = 0; r < numRows; r++){
                    if(!bitRead(GPIO_State, rowPins[r])){
                        uint8_t button = c + 1 + numCols * r;
                        _newState += power(2, button - 1);
                    }
                }
            }
            return _newState;
        }

        void updateDebouncedOutputState(){
            static int prevVal = 0;
            static int debounceCounter = 0;
            if(outputState == debouncedBooleanOutputState){
                debounceCounter = 0;
                return;
            }
            if(prevVal != outputState){
                debounceCounter = 0;
                prevVal = outputState;
            }
            else if(++debounceCounter > (outputState != 0 ? debounceCount : debounceCount * 3)){
                debouncedBooleanOutputState = outputState;
                updated = true;
                if(debouncedBooleanOutputState == errorVal){
                    debouncedOutputState = 255;
                    return;
                }
                debouncedOutputState = 0;
                for(int i = 0; i < numRows * numCols; i++){
                    if(bitRead(debouncedBooleanOutputState, i)){
                        debouncedOutputState = i + 1;
                        break;
                    }
                }
            }
        }
};