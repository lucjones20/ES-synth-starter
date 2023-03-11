#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <string>
#include <iostream>
#include <atomic>
#include "knob.cpp"
#include <vector>
#include <ES_CAN.h>
#include <cmath>
#include "adsr.cpp"



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
SemaphoreHandle_t CAN_TX_Semaphore;
Knob* volumeKnob;
Knob* octaveKnob;
//Step sizes
volatile uint32_t currentStepSize[12];
std::string keyPressed;
uint8_t TX_Message[8];
struct Note {
    std::string name;
    uint32_t stepSize;
};

typedef struct KnobParameters{
    Knob* volumeKnob;
    Knob* octaveKnob;
} KnobParameters;

int fs = 22000;
int f = 440;
double factor = 1.05946309436;
int octaveNumber;
QueueHandle_t msgOutQ;
QueueHandle_t msgInQ;


// const Note stepSizes[12] = {
Note stepSizes[12] = {
    // C
    {"C", uint32_t(4294967296 * f / factor / factor / factor / factor / factor / factor / factor / factor / factor / fs)},
    // C#
    {"C#", uint32_t(4294967296 * f / factor / factor / factor / factor / factor / factor / factor / factor / fs)},
    // D
    {"D", uint32_t(4294967296 * f / factor / factor / factor / factor / factor / factor / factor / fs)},
    // D#
    {"D#", uint32_t(4294967296 * f / factor / factor / factor / factor / factor / factor / fs)},
    // E
    {"E", uint32_t(4294967296 * f / factor / factor / factor / factor / factor / fs)},
    // F
    {"F", uint32_t(4294967296 * f / factor / factor / factor / factor / fs)},
    // F#
    {"F#", uint32_t(4294967296 * f / factor / factor / factor / fs)},
    // G
    {"G", uint32_t(4294967296 * f / factor / factor / fs)},
    // G#
    {"G#", uint32_t(4294967296 * f / factor / fs)},
    // A
    {"A", uint32_t(4294967296 * f / fs)},
    // A#
    {"A#", uint32_t(4294967296 * f * factor / fs)},
    // B
   {"B", uint32_t(4294967296 * f * factor * factor / fs)}
};

double octaveFactors[9] = {1./16., 1./8., 1./4., 1./2., 1, 2, 4, 8, 16};

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
void addNodeToMsg(int note, bool press)
{
  if(press)
    TX_Message[0] = 'P';
  else
    TX_Message[0] = 'R';
  TX_Message[1] = uint8_t(octaveKnob->getCounter());
  TX_Message[2] = uint8_t(note);
}
std::pair<bool,bool> convertUint8ToPairs(uint8_t a, uint8_t b)
{
  std::pair<bool,bool> ret;
  if(a == 0b0)
    ret.first = true;
  else
    ret.first = false;
  if(b == 0b0)
    ret.second = true;
  else
    ret.second = false;
  return ret;
}
void generateMsg(volatile uint8_t*  currentKeys, uint8_t* prevKeys)
{
  std::vector<std::pair<bool,bool>> keyPairs;
  keyPairs.push_back(convertUint8ToPairs(*currentKeys & 0b1, *prevKeys & 0b01)); //Note 0
  keyPairs.push_back(convertUint8ToPairs(*currentKeys >> 1 & 0b1, *prevKeys >> 1 & 0b1)); //Note 1
  keyPairs.push_back(convertUint8ToPairs(*currentKeys >> 2 & 0b1, *prevKeys >> 2 & 0b1)); //Note 2
  keyPairs.push_back(convertUint8ToPairs(*currentKeys >> 3 & 0b1, *prevKeys >> 3 & 0b1)); //Note 3
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 1) & 0b1, *(prevKeys + 1) & 0b01)); //Note 4
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 1) >> 1 & 0b1, *(prevKeys + 1) >> 1 & 0b1)); //Note 5
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 1) >> 2 & 0b1, *(prevKeys + 1) >> 2 & 0b1)); //Note 6
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 1) >> 3 & 0b1, *(prevKeys + 1) >> 3 & 0b1)); //Note 7
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 2) & 0b1, *(prevKeys + 2) & 0b01)); //Note 8
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 2) >> 1 & 0b1, *(prevKeys + 2) >> 1 & 0b1)); //Note 9
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 2) >> 2 & 0b1, *(prevKeys + 2) >> 2 & 0b1)); //Note 10
  keyPairs.push_back(convertUint8ToPairs(*(currentKeys + 2) >> 3 & 0b1, *(prevKeys + 2) >> 3 & 0b1)); //Note 11
  for(int i = 0; i < 12; i++)
  {
    if(keyPairs[i].first == false && keyPairs[i].second == true)
    {
      TX_Message[0] = 'R';
      TX_Message[2] = uint8_t(i);
      xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
    }
    else if(keyPairs[i].first == true && keyPairs[i].second == false)
    {
      TX_Message[0] = 'P';
      TX_Message[2] = uint8_t(i);
      xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
    }
  }
}

ADSR* adsrKey1 = new ADSR();
ADSR* adsrKey2 = new ADSR();
ADSR* adsrKey3 = new ADSR();
ADSR* adsrKey4 = new ADSR();
ADSR* adsrKey5 = new ADSR();
ADSR* adsrKey6 = new ADSR();
ADSR* adsrKey7 = new ADSR();
ADSR* adsrKey8 = new ADSR();
ADSR* adsrKey9 = new ADSR();
ADSR* adsrKey10 = new ADSR();
ADSR* adsrKey11 = new ADSR();
ADSR* adsrKey12 = new ADSR();
ADSR* keyADSR[12] = {
  adsrKey1,
  adsrKey2,
  adsrKey3,
  adsrKey4,
  adsrKey5,
  adsrKey6,
  adsrKey7,
  adsrKey8,
  adsrKey9,
  adsrKey10,
  adsrKey11,
  adsrKey12
};

volatile uint8_t anyKeyPressed = 0;
void sampleISR() {
    static uint32_t phaseAcc[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int32_t tmpVout = 0;
    if(anyKeyPressed){
        for(int i = 0; i < 12; i++){
            if(currentStepSize[i] !=0){
                phaseAcc[i] += currentStepSize[i];
                tmpVout += ((phaseAcc[i] >> 24) - 128)*(keyADSR[i]->getAmplitude());
            }
        }
    }
    int32_t Vout = 0;
    Vout = tmpVout;
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
void scanKeysTask(void * pvParameters) {
    const TickType_t xFrequency = 50/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t prevKeyArray[3] = {0xF, 0xF, 0xF};
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
        uint32_t stepsize_local[12];
        std::fill(stepsize_local, stepsize_local + 12, 0);
        
        
        std::string keyPressed_local = "";
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        octaveKnob->advanceState((keyArray[4] & 0b0001) | (keyArray[4] & 0b0010));
        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 4; j++){
                if(!(((keyArray[i]) >> j) & 1)){
                    anyKeyPressed = 1;
                    keyPressed_local += stepSizes[j + 4*i].name; 
                    stepsize_local[j+4*i] = octaveFactors[octaveKnob->getCounter()] * stepSizes[j+4*i].stepSize;
                    keyADSR[j+4*i]->nextState(1);
                }
                else{
                    // keyADSR[j+4*i]->nextState(0);
                    stepsize_local[j+4*i] = 0;
                }
            }
        }


        // currentStepSize = (pow(2.0, 32) * 440 / 22000);
        if(keyArray[0] == 0xF && keyArray[1] == 0xF && keyArray[2] == 0xF){
            anyKeyPressed = 0;
            keyPressed_local = "";
            std::fill(stepsize_local, stepsize_local + 12, 0);
        }
        generateMsg(&(keyArray[0]), &prevKeyArray[0]);
        volumeKnob->advanceState((keyArray[3] & 0b0001) | (keyArray[3] & 0b0010));
        prevKeyArray[0] = keyArray[0];
        prevKeyArray[1] = keyArray[1];
        prevKeyArray[2] = keyArray[2];
        xSemaphoreGive(keyArrayMutex);
        
        uint32_t time_micro = micros();
        for(int i = 0; i < 12; i++){
            
            __atomic_store_n(&currentStepSize[i], stepsize_local[i], __ATOMIC_RELAXED);
            // uint32_t a = currentStepSize[i];
        }
        

        // __atomic_store_n(&currentStepSize, stepsize_local, __ATOMIC_RELAXED);
        // keyPressed = keyPressed_local;
    }
}

void displayUpdateTask(void * pvParameters){

    const TickType_t xFrequency = 100/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1){
        uint32_t volume= volumeKnob->getCounter();
        vTaskDelayUntil( &xLastWakeTime, xFrequency );

        //Update display
        u8g2.clearBuffer();                 // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
        u8g2.drawStr(2,10,"Helllo Daddy!");    // write something to the internal memory

        u8g2.setCursor(2,20);
        for(int i = 0; i < 3; i++){
            u8g2.print(keyArray[i],HEX);
        }
        u8g2.setCursor(30,20);
        u8g2.print("         OCTAVE: ");
        u8g2.print(octaveKnob->getCounter());
        
        u8g2.setCursor(2,30);
        u8g2.drawStr(2,30,keyPressed.c_str());
        // u8g2.drawStr(10,30, "           VOLUME" + str(((Knob *) pvParameters)->getState()));
        u8g2.setCursor(30,30);
        u8g2.print("          VOLUME: ");
        u8g2.print(volume);
        u8g2.sendBuffer();          // transfer internal memory to the display

        //Toggle LED
        digitalToggle(LED_BUILTIN);

    }
}

void CAN_TX_Task (void * pvParameters) {
	uint8_t msgOut[8];
	while (1) {
	xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
		xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
		CAN_TX(0x123, msgOut);
	}
}
void CAN_RX_Task(void* pvParameters){
  uint8_t msgIn[8];
  while(1)
  {
    xQueueReceive(msgInQ, msgIn, portMAX_DELAY);
    std::string local = "";
    if(msgIn[0] == 'P')
      local+= 'P';
    else
      local += 'R';
    local += ", Note" + std::to_string(msgIn[2]);
    keyPressed = local;
  }
}
void CAN_TX_ISR (void) {
	xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
}
void CAN_RX_ISR (void) {
	uint8_t RX_Message_ISR[8];
	uint32_t ID;
	CAN_RX(ID, RX_Message_ISR);
	xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
}

void setup() {
    // put your setup code here, to run once:
    CAN_Init(true);
    setCANFilter(0x123,0x7ff);
    CAN_Start();
    keyArrayMutex = xSemaphoreCreateMutex();
    msgOutQ = xQueueCreate(36,8);
    msgInQ = xQueueCreate(36,8);
    CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);
    CAN_RegisterTX_ISR(CAN_TX_ISR);
    CAN_RegisterRX_ISR(CAN_RX_ISR);
    volumeKnob = new Knob(0,8,5);
    octaveKnob = new Knob(0,8,4);
    volumeKnob = new Knob(0,8,5);
  
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
    NULL,			/* Parameter passed into the task */
    1,			/* Task priority */
    &displayUpdateHandle );	/* Pointer to store the task handle */
    TaskHandle_t CAN_TXTask = NULL;
    xTaskCreate(
      CAN_TX_Task,
      "CAN_Transmit",
      64,
      NULL,
      5,
      &CAN_TXTask);
    TaskHandle_t CAN_RXTask = NULL;
    xTaskCreate(
      CAN_RX_Task,
      "CAN_Recieve",
      64,
      NULL,
      5,
      &CAN_RXTask);

    vTaskStartScheduler();
}




void loop() {
}