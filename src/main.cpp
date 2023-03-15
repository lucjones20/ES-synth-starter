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
// #include "pianoadsr.cpp"
#include <map>



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
  bool expected = true;




Knob* volumeKnob;
Knob* octaveKnob;
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

//Maps for sound production:
std::map<uint8_t, std::atomic<uint32_t>> currentStepMap;
std::map<uint8_t, uint32_t> phaseAccMap;
std::map<uint8_t, std::atomic<uint32_t>>::iterator it;
std::map<uint8_t, std::pair<uint8_t,uint8_t>> amplitudeMap;
std::atomic<bool> mapFlag;


//Recording stuff
struct Recording{
  std::vector<std::pair<uint8_t,uint16_t>> keyStrokes;
  int curIndex = 0;
  void addKeyStroke(uint8_t index)
  {
    if(keyStrokes.size() != 0)
      keyStrokes.push_back(std::pair<uint8_t,uint16_t>(index, uint16_t(millis()) - keyStrokes[keyStrokes.size() - 1].second));
    else 
      keyStrokes.push_back(std::pair<uint8_t,uint16_t>(index, uint16_t(millis())));
  }
};
HardwareTimer *sampleTimer;
enum Mode{Solo, MultiMaster, MultiSlave, MultiRecordOn, MutliRecordPlayback};
std::vector<Recording*> recordings;
std::atomic<int> recordIndex;

std::atomic<Mode> control;

//Mutex
SemaphoreHandle_t keyArrayMutex;
SemaphoreHandle_t CAN_TX_Semaphore;
SemaphoreHandle_t recordMutex;
QueueHandle_t msgOutQ;
QueueHandle_t msgInQ;

//Enviroment control variables
bool isMultiple = false;


bool nextAmplitude(std::pair<uint8_t,uint8_t>* _state){
  switch (_state->second)
  {
    case 0b10:
      _state->second = 0b11;
      return true;
    case 0b11:
      if(_state->first <= 10) return false;
      else _state->first -= 2;
      break;
    case 0b01:
      if(_state->first <= 10 || _state->first > 64) return false;
      else _state->first -= 6;
      break;
    default: break;
  }
  return true;
}


int fs = 22000;
int f = 440;
double factor = 1.05946309436;
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

// std::vector<pianoADSR> keyADSR(12);
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
void generateMsg(volatile uint8_t*  currentKeys, uint8_t* prevKeys)
{
  TX_Message[1] = uint8_t(octaveKnob->getCounter());
  for (int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
    {
      bool cBit = (*(currentKeys + i) >> j & 0b1) == 0b0;
      bool pBit = (*(prevKeys + i) >> j & 0b1) == 0b0;
      if(!cBit && pBit)
      {
        TX_Message[0] = 'R';
        TX_Message[2] = uint8_t(i * 4 + j);
        xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
        if(amplitudeMap.find(octaveKnob->getCounter() * 12 + i * 4 + j) != amplitudeMap.end())
          amplitudeMap[octaveKnob->getCounter() * 12 + i * 4 + j] = std::pair<uint8_t,uint8_t>(amplitudeMap[octaveKnob->getCounter() * 12 + i * 4 + j].first, 0b01);
        if(control.load() == MultiRecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            recordings[recordIndex]->addKeyStroke(TX_Message[2] + TX_Message[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
      
      else if(cBit && !pBit)
      {
        TX_Message[0] = 'P';
        TX_Message[2] = uint8_t(i *4 + j);
        xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
        phaseAccMap[octaveKnob->getCounter() *12 + i *4 + j] = 0;
        amplitudeMap[octaveKnob->getCounter() * 12 + i * 4 + j] = std::pair<uint8_t,uint8_t>(64, 0b10);
        currentStepMap[octaveKnob->getCounter() * 12 + i * 4 + j] = octaveFactors[octaveKnob->getCounter()] * stepSizes[i *4 + j].stepSize;
        if(control.load() == MultiRecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            recordings[recordIndex]->addKeyStroke(TX_Message[2] + TX_Message[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
    }
}


void sampleISR() {
  int16_t Vout = 0;
  if(mapFlag.compare_exchange_weak(expected, false))
  {
    for(it = currentStepMap.begin(); it != currentStepMap.end(); it++)
    {
      phaseAccMap[it->first] += it->second;
      Vout += ((phaseAccMap[it->first] >> 24))*(static_cast<float>(amplitudeMap[it->first].first)/64)-128;
    }
    mapFlag = true;
    Vout = Vout >> (8 - volumeKnob->getCounter());
    analogWrite(OUTR_PIN, Vout + 128);
  }
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
    const TickType_t xFrequency = 75/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t prevKeyArray[3] = {0xF, 0xF, 0xF};
    while(1){
        for(int i = 0; i < 7; i++){
            // Only 0-2 for now as thats where keys are - will be expanded to 0 - 7
            // Delay for parasitic capacitance
            setRow(i);
            delayMicroseconds(3);
            xSemaphoreTake(keyArrayMutex, portMAX_DELAY);

            keyArray[i] = readCols();
            xSemaphoreGive(keyArrayMutex);
        }
        
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        octaveKnob->advanceState((keyArray[4] & 0b0001) | (keyArray[4] & 0b0010));


        if(mapFlag.compare_exchange_weak(expected, false))
        {  
          generateMsg(&(keyArray[0]), &prevKeyArray[0]);
          for(auto ite = amplitudeMap.begin(); ite != amplitudeMap.end(); ite++)
            if(!nextAmplitude(&ite->second))
            {
              currentStepMap.erase(currentStepMap.find(ite->first));
              phaseAccMap.erase(phaseAccMap.find(ite->first));
              ite = amplitudeMap.erase(amplitudeMap.find(ite->first));
              if(ite == amplitudeMap.end())
                break;
            }
          mapFlag = true;
        }
        volumeKnob->advanceState((keyArray[3] & 0b0001) | (keyArray[3] & 0b0010));
        prevKeyArray[0] = keyArray[0];
        prevKeyArray[1] = keyArray[1];
        prevKeyArray[2] = keyArray[2];
        xSemaphoreGive(keyArrayMutex);        
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
    {
      local+= 'P';
      if(isMultiple)
      {
        while(!mapFlag.compare_exchange_weak(expected,false));
        phaseAccMap[msgIn[2] + msgIn[1] * 12] = 0;
        currentStepMap[msgIn[2] + msgIn[1] * 12] = stepSizes[msgIn[2]].stepSize * octaveFactors[octaveKnob->getCounter()];
        amplitudeMap[msgIn[2] + msgIn[1] * 12] = std::pair<uint8_t,uint8_t>(64, 0b10);
        mapFlag = true;
        if(control.load() == MultiRecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            recordings[recordIndex]->addKeyStroke(msgIn[2] + msgIn[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
    }
    else
    {
      local += 'R';
      if(isMultiple)
      {
        while(!mapFlag.compare_exchange_weak(expected,false));
        if(amplitudeMap.find(msgIn[2] + msgIn[1] * 12) != amplitudeMap.end())
          amplitudeMap[msgIn[2] + msgIn[1] * 12] = std::pair<uint8_t, uint8_t>(amplitudeMap[msgIn[2] + msgIn[1] * 12].first, 0b01);
        mapFlag = true;
        if(control.load() == MultiRecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            recordings[recordIndex]->addKeyStroke(msgIn[2] + msgIn[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
      
    }
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
    mapFlag = true;
    control = Solo;
    CAN_Init(!isMultiple);
    setCANFilter(0x123,0x7ff);
    CAN_Start();
    keyArrayMutex = xSemaphoreCreateMutex();
    recordMutex = xSemaphoreCreateMutex();
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