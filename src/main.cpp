#include <Arduino.h>
#include <U8g2lib.h>
#include <iostream>
#include <STM32FreeRTOS.h>


//Constants
const uint32_t interval = 100; //Display update interval
volatile int32_t currentStepSize;

//Pin definitions
//Row select and enable
const int RA0_PIN = D3;
const int RA1_PIN = D6;
const int RA2_PIN = D12;
const int REN_PIN = A5;

//Matrix input and output
const int C0_PIN = A2;
const int C1_PIN = D9;
const int C2_PIN = A6;
const int C3_PIN = D1;
const int OUT_PIN = D11;

//Audio analogue out
const int OUTL_PIN = A4;
const int OUTR_PIN = A3;

//Joystick analogue in
const int JOYY_PIN = A0;
const int JOYX_PIN = A1;

//Output multiplexer bits
const int DEN_BIT = 3;
const int DRST_BIT = 4;
const int HKOW_BIT = 5;
const int HKOE_BIT = 6;

//Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

int fs = 22000;
int f = 440;
double factor = 1.05946309436;
const int32_t stepSize[] = {
    4294967296 * f / factor / factor / factor / factor / factor / factor / factor / factor / factor / fs,
    4294967296 * f / factor / factor / factor / factor / factor / factor / factor / factor / fs,
    4294967296 * f / factor / factor / factor / factor / factor / factor / factor / fs,
    4294967296 * f / factor / factor / factor / factor / factor / factor / fs,
    4294967296 * f / factor / factor / factor / factor / factor / fs,
    4294967296 * f / factor / factor / factor / factor / fs,
    4294967296 * f / factor / factor / factor / fs,
    4294967296 * f / factor / factor / fs,
    4294967296 * f / factor / fs,
    4294967296 * f / fs,
    4294967296 * f * factor / fs,
    4294967296 * f * factor * factor / fs,
};

const char* notes[] = {
"C",
"C#",
"D",
"D#",
"E",
"F",
"F#",
"G",
"G#",
"A",
"A#",
"B"
};


//Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value) {
    digitalWrite(REN_PIN,LOW);
    digitalWrite(RA0_PIN, bitIdx & 0x01);
    digitalWrite(RA1_PIN, bitIdx & 0x02);
    digitalWrite(RA2_PIN, bitIdx & 0x04);
    digitalWrite(OUT_PIN,value);
    digitalWrite(REN_PIN,HIGH);
    delayMicroseconds(2);
    digitalWrite(REN_PIN,LOW);
}

uint8_t readCols(){
    return uint8_t((
        (digitalRead(C0_PIN) & 0b1) << 3)
        + ((digitalRead(C1_PIN) & 0b1) << 2) 
        + ((digitalRead(C2_PIN) & 0b1) << 1)
        + ((digitalRead(C3_PIN) & 0b1))
    );
}

void setRow(uint8_t rowIdx){
    int ra0 = (rowIdx & 0b1);
    int ra1 = ((rowIdx >> 1) & 0b1);
    int ra2 = ((rowIdx >> 2) & 0b1);

    digitalWrite(RA0_PIN, ra0);
    digitalWrite(RA1_PIN, ra1);
    digitalWrite(RA2_PIN, ra2);
    digitalWrite(REN_PIN, HIGH);
}

void sampleISR(){
    static int32_t phaseAcc = 0;
    phaseAcc += currentStepSize;
    int32_t Vout = phaseAcc >> 26;
    analogWrite(OUTR_PIN, Vout + 128);
}


void setup() {
    // put your setup code here, to run once:

    //Set pin directions
    pinMode(RA0_PIN, OUTPUT);
    pinMode(RA1_PIN, OUTPUT);
    pinMode(RA2_PIN, OUTPUT);
    pinMode(REN_PIN, OUTPUT);
    pinMode(OUT_PIN, OUTPUT);
    pinMode(OUTL_PIN, OUTPUT);
    pinMode(OUTR_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    pinMode(C0_PIN, INPUT);
    pinMode(C1_PIN, INPUT);
    pinMode(C2_PIN, INPUT);
    pinMode(C3_PIN, INPUT);
    pinMode(JOYX_PIN, INPUT);
    pinMode(JOYY_PIN, INPUT);

    //Initialise display
    setOutMuxBit(DRST_BIT, LOW);  //Assert display logic reset
    delayMicroseconds(2);
    setOutMuxBit(DRST_BIT, HIGH);  //Release display logic reset
    u8g2.begin();
    setOutMuxBit(DEN_BIT, HIGH);  //Enable display power supply

    //Initialise UART
    Serial.begin(9600);
    Serial.println("Hello World");


    TIM_TypeDef *Instance = TIM1;
    HardwareTimer *sampleTimer = new HardwareTimer(Instance);

    sampleTimer->setOverflow(22000, HERTZ_FORMAT);
    sampleTimer->attachInterrupt(sampleISR);
    sampleTimer->resume();
}

volatile uint8_t keyArray[7];
const char* key = NULL;

void scanKeysTask(void * pvParamters){
        while(1){
            const int range = 3;
            // uint8_t keys = readCols();

            for(int i = 0; i < range; i++){
                setRow(uint8_t(i));
                delayMicroseconds(5);
                keyArray[i] = readCols();
            }
            currentStepSize = 0;
            for(int i = 0; i < range; i++){
                for(int j = 0; j < 4; j++){
                    if(!((keyArray[i] >> (3-j)) & 0b1)){
                        key = notes[(i*4) + j]; 
                        __atomic_store_n(&currentStepSize, stepSize[(i*4 + j)], __ATOMIC_RELAXED);
                        // currentStepSize = stepSize[(i*4 + j)];
                    }
                }
            }
        }
}

void loop() {
    // put your main code here, to run repeatedly:
    static uint32_t next = millis();
    static uint32_t count = 0;


    if (millis() > next) {
        next += interval;

        scanKeysTask(NULL);

        //Update display
        u8g2.clearBuffer();         // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
        u8g2.drawStr(2,10,"Hello World!");  // write something to the internal memory
        u8g2.setCursor(2,20);
        u8g2.print(key);
        // u8g2.print(keyArray[0],HEX);
        // u8g2.print(keyArray[1],HEX);
        // u8g2.print(keyArray[2],HEX);
        u8g2.sendBuffer();          // transfer internal memory to the display

        //Toggle LED
        digitalToggle(LED_BUILTIN);

    }
}