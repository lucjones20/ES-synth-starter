#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <string>
#include <iostream>
#include <atomic>
#include "knob.cpp"



//Constants
    const uint32_t interval = 100; //Display update interval

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


//Mutex
SemaphoreHandle_t keyArrayMutex;
Knob* volumeKnob;
//Step sizes
volatile uint32_t currentStepSize;
std::string keyPressed;

struct Note {
    std::string name;
    uint32_t stepSize;
};



// const Note stepSizes[12] = {
Note stepSizes[12] = {
    // C
    {"C", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -9.0 / 12))},
    // C#
    {"C#", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -8.0 / 12))},
    // D
    {"D", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -7.0 / 12))},
    // D#
    {"D#", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -6.0 / 12))},
    // E
    {"E", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -5.0 / 12))},
    // F
    {"F", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -4.0 / 12))},
    // F#
    {"F#", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -3.0 / 12))},
    // G
    {"G", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, -2.0 / 12))},
    // G#
    {"G#", uint32_t((pow(2.0, 32) * 440 / 22000)* pow(2.0, -1.0 / 12))},
    // A
    {"A", uint32_t(pow(2.0, 32) * 440 / 22000)},
    // A#
    {"A#", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, 1.0 / 12))},
    // B
   {"B", uint32_t((pow(2.0, 32) * 440 / 22000) * pow(2.0, 2.0 / 12))}
};



//Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

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

void sampleISR() {
    static uint32_t phaseAcc = 0;
    phaseAcc += currentStepSize;

    int32_t Vout = (phaseAcc >> 24) - 128;
    Vout = Vout >> (8 - volumeKnob->getCounter());
    analogWrite(OUTR_PIN, Vout + 128);
}

void setRow(uint8_t rowIdx){
	//Disable the row select enable (REN_PIN) first to prevent glitches as the address pins are changed
	digitalWrite(REN_PIN, LOW);

	digitalWrite(RA0_PIN, LOW);
	digitalWrite(RA1_PIN, LOW);
  digitalWrite(RA2_PIN, LOW);

	
	if(rowIdx & 0b1){
		digitalWrite(RA0_PIN, HIGH);
	}
	if(rowIdx & 0b10){
		digitalWrite(RA1_PIN, HIGH);
	}
	if(rowIdx & 0b100){
		digitalWrite(RA2_PIN, HIGH);
	}


	digitalWrite(REN_PIN, HIGH);
}

uint8_t readCols(){

	uint8_t C0 = digitalRead(C0_PIN);
	uint8_t C1 = digitalRead(C1_PIN);
	uint8_t C2 = digitalRead(C2_PIN);
	uint8_t C3 = digitalRead(C3_PIN);

	uint8_t result = (C3 << 3) | (C2 << 2) | (C1 << 1) | C0;

	return result;
}

volatile uint8_t keyArray[7];
long ctime = 0;
void scanKeysTask(void * pvParameters) {
    const TickType_t xFrequency = 10/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1){
        for(int i = 0; i < 8; i++){
            // Only 0-2 for now as thats where keys are - will be expanded to 0 - 7
            // Delay for parasitic capacitance
            setRow(i);
            delayMicroseconds(3);
            xSemaphoreTake(keyArrayMutex, portMAX_DELAY);

            keyArray[i] = readCols();
            xSemaphoreGive(keyArrayMutex);
        }
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        uint32_t stepsize_local;
        std::string keyPressed_local;
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 4; j++){
                if(!(((keyArray[i]) >> j) & 1)){
                    // std::cout<< ((keyArray[i]) >> j);
                    keyPressed_local = stepSizes[j + 4*i].name; 
                    stepsize_local = stepSizes[j+4*i].stepSize;
                }
            }
        }

        // currentStepSize = (pow(2.0, 32) * 440 / 22000);
        if(keyArray[0] == 0xF && keyArray[1] == 0xF && keyArray[2] == 0xF){
            keyPressed_local = "";
            stepsize_local = 0;
        }
        volumeKnob->advanceState((keyArray[3] & 0b0001) | (keyArray[3] & 0b0010));

        xSemaphoreGive(keyArrayMutex);

        __atomic_store_n(&currentStepSize, stepsize_local, __ATOMIC_RELAXED);
        keyPressed = keyPressed_local;
    }
}

void displayUpdateTask(void * pvParameters){
    

    const TickType_t xFrequency = 100/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1){
        vTaskDelayUntil( &xLastWakeTime, xFrequency );

        static uint32_t next = millis();
        static uint32_t count = 0;


        next += interval;

        //Update display
        u8g2.clearBuffer();                 // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
        u8g2.drawStr(2,10,"Helllo World!");    // write something to the internal memory

        u8g2.setCursor(2,20);
        for(int i = 0; i < 3; i++){
            u8g2.print(keyArray[i],HEX);
        }
        
        u8g2.setCursor(2,30);
        u8g2.drawStr(2,30,keyPressed.c_str());
        // u8g2.drawStr(10,30, "           VOLUME" + str(((Knob *) pvParameters)->getState()));
        u8g2.setCursor(30,30);
        u8g2.print(((Knob *) pvParameters)->getState());
        u8g2.sendBuffer();          // transfer internal memory to the display

        //Toggle LED
        digitalToggle(LED_BUILTIN);

    }
}

void setup() {
    // put your setup code here, to run once:
    keyArrayMutex = xSemaphoreCreateMutex();
    volumeKnob = new Knob(0, 8, 8);
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
    setOutMuxBit(DRST_BIT, LOW);    //Assert display logic reset
    delayMicroseconds(2);
    setOutMuxBit(DRST_BIT, HIGH);    //Release display logic reset
    u8g2.begin();
    setOutMuxBit(DEN_BIT, HIGH);    //Enable display power supply

    //Timer to trigger interrupt (sampleISR() function)
    TIM_TypeDef *Instance = TIM1;
    HardwareTimer *sampleTimer = new HardwareTimer(Instance);
    sampleTimer->setOverflow(22000, HERTZ_FORMAT);
    sampleTimer->attachInterrupt(sampleISR);
    sampleTimer->resume();
    
    //Initialise UART
    Serial.begin(9600);
    Serial.println("Hello World");

    TaskHandle_t scanKeysHandle = NULL;
    xTaskCreate(
    scanKeysTask,		/* Function that implements the task */
    "scanKeys",		/* Text name for the task */
    64,      		/* Stack size in words, not bytes */
    NULL,			/* Parameter passed into the task */
    2,			/* Task priority */
    &scanKeysHandle );	/* Pointer to store the task handle */

    TaskHandle_t displayUpdateHandle = NULL;
    xTaskCreate(
    displayUpdateTask,		/* Function that implements the task */
    "displayUpdate",		/* Text name for the task */
    256,      		/* Stack size in words, not bytes */
    volumeKnob,			/* Parameter passed into the task */
    1,			/* Task priority */
    &displayUpdateHandle );	/* Pointer to store the task handle */

    vTaskStartScheduler();
}




void loop() {
}