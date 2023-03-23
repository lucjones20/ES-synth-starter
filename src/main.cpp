#ifndef main_cpp
#define main_cpp
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
#include <set>
// #include "pianoadsr.cpp"
#include <map>
#include <main.hpp>
#include <algorithm>

#include "menu.cpp"

//Test switches:
bool disable_blocks = false;
// #define DISABLE_THREADS
// #define DISABLE_INTERRUPT
// #define TEST_SCANKEYS
// #define TEST_DISPLAY
// #define TEST_SAMPLEISR
// #define TEST_COMMUNICATION_TASKS



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




//Knobs
Knob* volumeKnob;
Knob* octaveKnob;
Knob* menuKnob;
Knob* waveformKnob;

//Key array and note 
volatile uint8_t keyArray[7];
std::string notesPlayed;
//Waveform arrays
SineArray* sineArray;
volatile uint32_t array[108];
volatile int triangleCoeff[88];

//For transmit messages and record incoming mesage to display
std::string keyPressed;
uint8_t TX_Message[8];

//Maps for sound production:
std::map<uint8_t, std::atomic<uint32_t>> currentStepMap;
volatile uint32_t phaseAccArray[96];
volatile uint8_t amplitudeState[96];
volatile uint8_t amplitudeAmp[96];
std::atomic<bool> mapFlag;





//Mutex
SemaphoreHandle_t keyArrayMutex;
SemaphoreHandle_t CAN_TX_Semaphore;
SemaphoreHandle_t recordMutex;
QueueHandle_t msgOutQ;
QueueHandle_t msgInQ;

//Enviroment control variables
bool isMultiple = true;
bool isReciever = false;

//ADSR State machine
bool nextAmplitude(volatile uint8_t* state,volatile uint8_t* amplitude){
  switch (*state)
  {
    case 0b10:  //cBit|pBit
      *state = 0b11;
      return true;
    case 0b11:
      if(*amplitude <= 10) return false;
      else *amplitude -= 0.66;
      break;
    case 0b01:
      if(*amplitude <= 10 || *amplitude > 64) return false;
      else *amplitude -= 2;
      break;
    default: break;
  }
  return true;
}


//Pre stored values for the notes
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

//Adds a keypress/keyrelease to the recordign
void addKeyStrokeToRecording(Recording* rec,uint8_t index)
  {
    
    if(rec->keyStrokes.size() != 0)
    {
      
      rec->keyStrokes.push_back(std::pair<uint8_t,uint16_t>(index, uint16_t(millis()) - rec->lastPressed));
      rec->lastPressed = millis();
    }
    else 
    {
      rec->keyStrokes.push_back(std::pair<uint8_t,uint16_t>(index, 0));
      rec->lastPressed = millis();
    }
  }

//Generates the next msg for playback
void nextStepToPlayback(Recording* rec)
  {

    if(rec->curIndex == rec->keyStrokes.size())
      rec->curIndex = 0;

    int shift = rec->keyStrokes[rec->curIndex].first/12 - 4;
    
    if(millis() - rec->lastPressed > rec->keyStrokes[rec->curIndex].second)
    {
      if(rec->pressed.find(rec->keyStrokes[rec->curIndex].first) != rec->pressed.end())
      {

        rec->pressed.erase(rec->keyStrokes[rec->curIndex].first);
        amplitudeState[rec->keyStrokes[rec->curIndex].first] = 0b01;

      
      }
      else
      {

        amplitudeAmp[rec->keyStrokes[rec->curIndex].first] = 64;
        amplitudeState[rec->keyStrokes[rec->curIndex].first] = 0b10;
        currentStepMap[rec->keyStrokes[rec->curIndex].first] = (shift >= 0) ? (stepSizes[rec->keyStrokes[rec->curIndex].first%12].stepSize << shift) : (stepSizes[rec->keyStrokes[rec->curIndex].first%12].stepSize >> abs(shift));
        rec->pressed.insert(rec->keyStrokes[rec->curIndex].first);
      }
      rec->curIndex++;
      rec->lastPressed = millis();
    }
  }
//Recording vector and index
std::vector<Recording*> recordings;
std::atomic<int> recordIndex;



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

//Process the key array
void processKeys(volatile uint8_t*  currentKeys, uint8_t* prevKeys)
{

  TX_Message[1] = uint8_t(octaveKnob->getCounter()); //Set octave
  //If playback add playback msg
  if(Menu::getMode() == PlaybackOn)
    nextStepToPlayback(recordings[recordIndex]);
  for (int i = 0; i < 3; i++)
    for(int j = 0; j < 4; j++)
    {
      bool cBit = (*(currentKeys + i) >> j & 0b1) == 0b0;
      bool pBit = (*(prevKeys + i) >> j & 0b1) == 0b0;
      if(!cBit && pBit) //If key release
      {
        TX_Message[0] = 'R';
        TX_Message[2] = uint8_t(i * 4 + j);
        xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
        amplitudeState[octaveKnob->getCounter() * 12 + i * 4 + j] = 0b01;
        if(Menu::getMode() == RecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            addKeyStrokeToRecording(recordings[recordIndex],TX_Message[2] + TX_Message[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
      
      else if(cBit && !pBit) //If key press
      {
        TX_Message[0] = 'P';
        TX_Message[2] = uint8_t(i *4 + j);
        xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
        amplitudeAmp[octaveKnob->getCounter() * 12 + i * 4 + j] = 64;
        amplitudeState[octaveKnob->getCounter() * 12 + i * 4 + j] = 0b10;
        int shift = octaveKnob->getCounter() - 4;
        currentStepMap[octaveKnob->getCounter() * 12 + i * 4 + j] = (shift >= 0) ? (stepSizes[i *4 + j].stepSize << shift) : (stepSizes[i *4 + j].stepSize >> abs(shift));
        if(Menu::getMode() == RecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            addKeyStrokeToRecording(recordings[recordIndex], TX_Message[2] + TX_Message[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
    }
}


void sampleISR() {
  static int sineCounter[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
  int32_t Vout = 0;
  bool expected = true;

  if(mapFlag.compare_exchange_weak(expected, false))
  {
    if(!isMultiple || (isMultiple && isReciever)){
      
      for(auto it = currentStepMap.begin(); it != currentStepMap.end(); it++)
      {
        if(waveformKnob->getCounter()==1){
          switch(octaveKnob->getCounter()){
            case 1:
              phaseAccArray[it->first] = sineArray->octave1[it->first%12][(sineCounter[it->first%12]++)%400]; 
              break;
            case 2:
              phaseAccArray[it->first] = sineArray->octave2[it->first%12][(sineCounter[it->first%12]++)%200]; 
              break;
            case 3:
              phaseAccArray[it->first] = sineArray->octave3[it->first%12][(sineCounter[it->first%12]++)%100]; 
              break;            
            case 4:
              phaseAccArray[it->first] = sineArray->octave4[it->first%12][(sineCounter[it->first%12]++)%50]; 
              break;
            case 5:
              phaseAccArray[it->first] = sineArray->octave5[it->first%12][(sineCounter[it->first%12]++)%25]; 
              break;
            case 6:
              phaseAccArray[it->first] = sineArray->octave6[it->first%12][(sineCounter[it->first%12]++)%13]; 
            case 7:
              phaseAccArray[it->first] = sineArray->octave6[it->first%12][(sineCounter[it->first%12]++)%7]; 
              break;
          }
          Vout += ((phaseAccArray[it->first] >> 24))*((float)amplitudeAmp[it->first]/(float)64)-128;
        }
        else if(waveformKnob->getCounter()==2){
          if(phaseAccArray[it->first] + triangleCoeff[it->first] * 2 * it->second < phaseAccArray[it->first]){
            triangleCoeff[it->first] = -1;
          }
          else{
            triangleCoeff[it->first] = 1;
          }
          phaseAccArray[it->first] += triangleCoeff[it->first] * 2 * it->second;

          Vout += ((phaseAccArray[it->first] >> 24))*((float)amplitudeAmp[it->first]/(float)64)-128;
        }

        else if(waveformKnob->getCounter()==3)  //Implementing Square Wave (WIP)
        {
          
        phaseAccArray[it->first] += it->second;

        if (phaseAccArray[it->first] <= pow(2,31))
        {
          Vout += (-127)*((float)amplitudeAmp[it->first]/(float)64); //LOW
        }
        else
        {
          Vout += 127*((float)amplitudeAmp[it->first]/(float)64); //HIGH
        }



        }

        else{ //Sawtooth, default
          phaseAccArray[it->first] += it->second;

          Vout += ((phaseAccArray[it->first] >> 24))*((float)amplitudeAmp[it->first]/(float)64)-128;
        }
    }

    }
    mapFlag = true;
    Vout = Vout >> (8 - volumeKnob->getCounter());
    if(Vout > 127){
      Vout = 127;
    }
    if(Vout < -127){
      Vout = -127;
    }
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


void scanKeysTask(void * pvParameters) {
    //Init of task
    const TickType_t xFrequency = 20/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t prevKeyArray[3] = {0xF, 0xF, 0xF};
    uint8_t analysisCounter = 1;
    do{
        if(!disable_blocks)  
          vTaskDelayUntil( &xLastWakeTime, xFrequency );
        //Reading key array
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        for(int i = 0; i < 7; i++){
            setRow(i);
            delayMicroseconds(3);
            

            keyArray[i] = readCols();
        }
        volumeKnob->advanceState((keyArray[3] & 0b0010) | (keyArray[3] & 0b0001));
        octaveKnob->advanceState((keyArray[4] & 0b0010) | (keyArray[4] & 0b0001));
        menuKnob->advanceState((keyArray[3] & 0b0100) >> 2 | (keyArray[3] & 0b1000) >> 2);
        waveformKnob->advanceState((keyArray[4] & 0b0100)>> 2 | (keyArray[4] & 0b1000) >> 2);
        std::string currentNote_local = "";
        bool expected = true;
        if(mapFlag.compare_exchange_weak(expected, false))
        {  
          processKeys(&(keyArray[0]), &prevKeyArray[0]);
          for(auto it = currentStepMap.begin(); it != currentStepMap.end(); it++)
          {
            if(!nextAmplitude(&amplitudeState[it->first], &amplitudeAmp[it->first]))
            {
              currentStepMap.erase(it);
            }
            currentNote_local += stepSizes[it->first%12].name;
          }
          mapFlag = true;
        }
        
        prevKeyArray[0] = keyArray[0];
        prevKeyArray[1] = keyArray[1];
        prevKeyArray[2] = keyArray[2];
        
        Menu::updateMenu(!(keyArray[6] & 0b1), !(keyArray[6] & 0b10), !(keyArray[5] & 0b1), !(keyArray[5] & 0b10), menuKnob->getCounter());
        recordIndex = Menu::getRecordIndex();
        notesPlayed = currentNote_local;
        xSemaphoreGive(keyArrayMutex);    
    }while(!disable_blocks);
}

void displayUpdateTask(void * pvParameters){

    const TickType_t xFrequency = 100/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    do{
        uint32_t volume= volumeKnob->getCounter();
        if(!disable_blocks)
          vTaskDelayUntil( &xLastWakeTime, xFrequency );

        //Update display
        u8g2.clearBuffer();                 // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
        u8g2.setCursor(2,10);
        u8g2.print("Mode:");
        switch(Menu::getMode())
        {
          case Normal:
            u8g2.print("Normal");
            xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
            u8g2.setCursor(2,20);
            // for(int i = 0; i < 3; i++){
            //     u8g2.print(keyArray[i],HEX);
            // }
            u8g2.drawStr(2, 20, notesPlayed.c_str());
            xSemaphoreGive(keyArrayMutex);
            break;
          case RecordOff:
            u8g2.print("RecOff");
            u8g2.setCursor(5,20);
            u8g2.print("Track:");
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            u8g2.print(recordIndex.load());
            xSemaphoreGive(recordMutex);
            break;
          case RecordOn:
            u8g2.print("RecOn");
            u8g2.setCursor(5,20);
            u8g2.print("Track:");
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            u8g2.print(recordIndex.load());
            u8g2.print("     Pressed:");
            u8g2.print(recordings[recordIndex]->keyStrokes.size());
            xSemaphoreGive(recordMutex);
            break;
          case PlaybackOff:
            u8g2.print("PlayOff");
            u8g2.setCursor(5,20);
            u8g2.print("Track:");
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            u8g2.print(recordIndex.load());
            u8g2.print("/");
            u8g2.print(recordings.size()-1);
            xSemaphoreGive(recordMutex);
            break;
          case PlaybackOn:
            u8g2.print("PlayOn");
            u8g2.setCursor(5,20);
            u8g2.print("Track:");
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            u8g2.print(recordIndex.load());
            xSemaphoreGive(recordMutex);
        }   

        
        u8g2.setCursor(50,30);
        u8g2.print("W");
        u8g2.print(waveformKnob->getCounter());
        u8g2.print("   O:");  
        u8g2.print(octaveKnob->getCounter());
        
        u8g2.setCursor(2,30);
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        u8g2.drawStr(2,30,keyPressed.c_str());
        xSemaphoreGive(keyArrayMutex);
        u8g2.setCursor(100,30);
        u8g2.print("V:");
        u8g2.print(volume);
        u8g2.sendBuffer();// transfer internal memory to the display

        //Toggle LED
        digitalToggle(LED_BUILTIN);

    }while(!disable_blocks);
}

void CAN_TX_Task (void * pvParameters) {
	uint8_t msgOut[8];
	do{
	xQueueReceive(msgOutQ, msgOut, portMAX_DELAY);
		xSemaphoreTake(CAN_TX_Semaphore, portMAX_DELAY);
		CAN_TX(0x123, msgOut);
	}while(!disable_blocks);
}
void CAN_RX_Task(void* pvParameters){
  uint8_t msgIn[8];
  do
  {
    xQueueReceive(msgInQ, msgIn, portMAX_DELAY);
    std::string local = "";
    if(msgIn[0] == 'P')
    {
      local+= 'P';
      if(isMultiple)
      {
        bool expected = true;
        while(!mapFlag.compare_exchange_weak(expected,false)) expected = true;
        int shift = msgIn[1] - 4;
        currentStepMap[msgIn[2] + msgIn[1] * 12] = (shift >= 0) ? (stepSizes[msgIn[2]].stepSize << shift) : (stepSizes[msgIn[2]].stepSize >> abs(shift));
        amplitudeAmp[msgIn[2] + msgIn[1] * 12] = 64;
        amplitudeState[msgIn[2] + msgIn[1] * 12] = 0b10;
        mapFlag = true;
        if(Menu::getMode() == RecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            addKeyStrokeToRecording(recordings[recordIndex],msgIn[2] + msgIn[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
    }
    else
    {
      local += 'R';
      if(isMultiple)
      {
        bool expected = true;
        while(!mapFlag.compare_exchange_weak(expected,false)) expected = true;
        amplitudeState[msgIn[2] + msgIn[1] * 12] = 0b01;
        mapFlag = true;
        if(Menu::getMode() == RecordOn)
        {
            xSemaphoreTake(recordMutex, portMAX_DELAY);
            addKeyStrokeToRecording(recordings[recordIndex], msgIn[2] + msgIn[1]*12);
            xSemaphoreGive(recordMutex);
        }
      }
      
    }
    local += std::to_string(msgIn[2]); 
    keyPressed = local;
  }while(!disable_blocks);
}
void CAN_TX_ISR (void) {
  #ifdef DISABLE_INTERRUPT
  xSemaphoreGive(CAN_TX_Semaphore);
  #else
	xSemaphoreGiveFromISR(CAN_TX_Semaphore, NULL);
  #endif

}
void CAN_RX_ISR (void) {
	uint8_t RX_Message_ISR[8];
	uint32_t ID;
	CAN_RX(ID, RX_Message_ISR);
  #ifndef DISABLE_INTERRUPT
	xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
  #else
	xQueueSend(msgInQ, RX_Message_ISR, 0);
  #endif
}

void setup() {
    // put your setup code here, to run once:
    mapFlag = true;
    CAN_Init(!isMultiple);
    setCANFilter(0x123,0x7ff);
    CAN_Start();
    keyArrayMutex = xSemaphoreCreateMutex();
    recordMutex = xSemaphoreCreateMutex();
    CAN_TX_Semaphore = xSemaphoreCreateCounting(3,3);
    Menu::setupMen(&recordings, &recordMutex);
    #ifndef TEST_SCANKEYS
    msgOutQ = xQueueCreate(36,8);
    #endif
    #ifdef TEST_SCANKEYS
    msgOutQ = xQueueCreate(384,8);
    #endif
    msgInQ = xQueueCreate(36,8);


    volumeKnob = new Knob(0,8,5);
    octaveKnob = new Knob(0,8,4);
    menuKnob = new Knob(0, 3, 0);
    // waveformKnob = new Knob(0,2,1);

    waveformKnob = new Knob(0, 3, 0);   //increasing the number of waveforms range so it's easier to test...

    std::fill_n(triangleCoeff, 88, 1);
    
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
    #ifndef DISABLE_INTERRUPT
    TIM_TypeDef *Instance = TIM1;
    HardwareTimer *sampleTimer = new HardwareTimer(Instance);
    sampleTimer->setOverflow(22000, HERTZ_FORMAT);
    sampleTimer->attachInterrupt(sampleISR);
    sampleTimer->resume();
    CAN_RegisterTX_ISR(CAN_TX_ISR);
    CAN_RegisterRX_ISR(CAN_RX_ISR);
    #endif
    
    //Initialise UART
    Serial.begin(9600);
    Serial.println("Hello World");

    initSineArray(sineArray);

    #ifndef DISABLE_THREADS
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
      #endif

      #ifdef TEST_SCANKEYS
        uint32_t startTime = micros();
        for (int iter = 0; iter < 32; iter++) {
          scanKeysTask(NULL);
        }
        Serial.println(micros()-startTime);
        while(1);
      #endif
      #ifdef TEST_DISPLAY
        scanKeysTask(NULL);
        uint32_t startTime = micros();

        for (int iter = 0; iter < 32; iter++) {
          displayUpdateTask(NULL);
        }
        Serial.println(micros()-startTime);
        while(1);
      #endif
      #ifdef TEST_SAMPLEISR
        
        scanKeysTask(NULL);
        uint32_t startTime = micros();
        for (int iter = 0; iter < 32; iter++) {
          sampleISR();
        }
        Serial.println(micros()-startTime);
        while(1); 
      #endif
      #ifdef TEST_COMMUNICATION_TASKS
      uint32_t sendTimer = 0;
      uint32_t recievTimer = 0;
      uint32_t sendISR = 0;
      uint32_t recieveISR = 0;
      TX_Message[1] = 4;
      TX_Message[2] = 12;
      for(int i = 0; i < 36; i++)
      {
        if(i%2 == 0)
          TX_Message[0] = 'P';
        else
          TX_Message[0] = 'R';
        
        xQueueSend(msgOutQ, TX_Message, portMAX_DELAY);
        auto startTime = micros();
        CAN_TX_Task(NULL);
        sendTimer += micros() - startTime;
        startTime = micros();
        CAN_TX_ISR();
        sendISR += micros() - startTime;
        delay(100);
        startTime = micros();
        CAN_RX_ISR();
        recieveISR +=  micros() - startTime;
        startTime = micros();
        CAN_RX_Task(NULL);
        recievTimer +=  micros() - startTime;
      }
      Serial.print("Send Task:");
      Serial.println(sendTimer);
      Serial.print("Send Interrupt:");
      Serial.println(sendISR);
      Serial.print("Recieve Task:");
      Serial.println(recievTimer);
      Serial.print("Recieve ISR:");
      Serial.println(recieveISR);
      #endif
}




void loop() {
}
#endif
