volatile uint8_t anyKeyPressed = 0;
int8_t counter = 0;
int8_t counter2 = 0;
void sampleISR() {
    static uint32_t phaseAcc = 0;
    counter2++;
    if(counter2 >= 25) counter2 = 0;
    if (counter2 == 0){
        counter++;
        if(counter >= 12) counter = 0;
    }
    
    if(anyKeyPressed){
        for(int i = 0; i < 12; i++){
            if(currentStepSize[counter] !=0){
                phaseAcc += currentStepSize[counter];
                delayMicroseconds(10);
                break;
            }
            else{
                counter++;
                if(counter >= 12) counter = 0;
            }
        }
    }
    
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
void scanKeysTask(void * pvParameters) {
    const TickType_t xFrequency = 50/portTICK_PERIOD_MS;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    uint8_t prevKeyArray[3] = {0xF, 0xF, 0xF};
    while(1){
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
        uint32_t stepsize_local[12];
        std::fill(stepsize_local, stepsize_local + 12, 0);
        
        
        std::string keyPressed_local = "";
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 4; j++){
                if(!(((keyArray[i]) >> j) & 1)){
                    anyKeyPressed = 1;
                    keyPressed_local += stepSizes[j + 4*i].name; 
                    stepsize_local[j+4*i] = stepSizes[j+4*i].stepSize;
                }
                else{
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
        volumeKnob->advanceState((keyArray[3] & 0b0001) | (keyArray[3] & 0b0010));
        prevKeyArray[0] = keyArray[0];
        prevKeyArray[1] = keyArray[1];
        prevKeyArray[2] = keyArray[2];
        xSemaphoreGive(keyArrayMutex);
        
        uint32_t time_micro = micros();
        for(int i = 0; i < 12; i++){
            
            __atomic_store_n(&currentStepSize[i], stepsize_local[i], __ATOMIC_RELAXED);
            // uint32_t a = currentStepSize[i];
            // Serial.println(a);
        }
        

        // __atomic_store_n(&currentStepSize, stepsize_local, __ATOMIC_RELAXED);
        keyPressed = keyPressed_local;
    }
}