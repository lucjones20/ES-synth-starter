DOCUMENTATION
=============================

Table of Contents
-----------------------------

1. [Introduction](#introduction)
2. [Features](#features)    
    2.1. [Core features](#corefeatures)     
    2.2. [Advanced features](#advancedfeatures) 
3. [Tasks](#tasks)  
    3.1. [scanKeyTask](#scankeytask)  
    3.2. [displayUpdateTask](#displayupdatetask)    
    3.3. [sampleISR](#sampleISR)    
    3.3. [CAN_TX_Task](#cantxtask)  
    3.4. [CAN_RX_Task](#canrxtask) 
 4. [Analysis](#analysis)   

## 1. Introduction 

## 2. Features
### 2.1. Core features
- The synthesizer plays the correct music note with a sawtooth wave and without any delay
- There are 8 different volumes which you can control with a knob
- OLED display shows the current notes being played and the current volume 
- OLED display refreshes every 100ms and the LD3 LED toggles every 100ms
- The synthesizer can be configured as a sender or reciever at compile time
- The synthesizer will play the sounds if it is a reciever
- The synthesizer will send the note if it is a sender

### 2.2. Advanced features _(add Figures/Videos later when revising...)_
- a. Polyphony
    - When mul
- b. Octave Knob
    - In addition to the volume knob, Knob X was programmed to control the octave that the corresponding board/module plays in, ranging from 0-7.
- c. ADSR Envelopes
    - In order to provide further control to the sound production of the synthesiser, ADSR envelopes were implemented to modulate the evolution/progression of the sound. The four stages are as follows:

    - A - Attack: Once a key is pressed, the Attack phase controls the speed that the sound reaches full/max volume. It provides a delay factor that can be controlled.

    - D - Decay: After reaching max volume, the Decay phase decreases 

    - S - Sustain: 

    - R - Release: 

    - ADSR was implemented as... state machine...

    - each instruments..... 
- d. Additional Waveforms (Sine, Triangle, Sqaure)
    - In addition to the core sawtooth waveform, sine, triangle and square waveforms were implemented. The waveform can be selected and toggled/cycled through the use of Knob X.   

    - Sine: 
        - Range: 
        - Period:
        - Resolution: 
        - Implementation: LUT?  

    - Triangle:
        - Range: 
        - Period:
        - Resolution: 
        - Implementation:  

    - Square: 
        - Range: 
        - Period:
        - Resolution: 
        - Implementation: 


- e. Recording & Playback
    - ...

* _(*Not implemented/Not implemented yet)_
- _*Joystick_
    - *...
- _*Stero sound with multiple boards' speakers_
    - *...

## 3. Tasks
* _(For each task, include: method of implementation, thread or interrupt + theoretical minimum initiation interval and measured maximum execution time + a critical instant analysis of the rate monotonic scheduler, showing that all deadlines are met under worst-case conditions)_
### 3.1. ScanKeyTask
The ScanKeyTask function is responsible for reading the key matrix

## 4. Analysis
* _(Include, in this section, pt. 19, 20 & 21 of the *Documentation Specifications*)_
* _(Probably re-do these tests at the end...)_

ScanKeyTask: 8430 / 32 = 263 $\mu s$ 

displayUpdateTask: 554261 / 32 = 17 320 $\mu s$ 

sampleISR: 689 / 32 = 21.5 $\mu s$  
