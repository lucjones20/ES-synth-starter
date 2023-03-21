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
    3.3. [sampleISR]()
    3.3. [CAN_TX_Task](#cantxtask)  
    3.4. [CAN_RX_Taskk](#canrxtask) 
 4. [Analysis](#analysis)   

## 1. Introduction 

## 2. Features
### 2.1. Core features
- The synthesizer plays the correct music note with a sawtooth wave and without any delay
- There are 8 different volumes which you can control with a knob
- OLED display shows the 

### 2.2. Advanced features
- Polyphony...
- Octave Knob...
- ADSR...
- Additional waveforms (Sine, Triangle, Sqaure)...
- Recording & Playback...

* _(*Not implemented yet)_
- _*Joystick_
- _*Stero sound with multiple boards' speakers_

## 3. Tasks
* _(For each task, include: method of implementation, thread or interrupt + theoretical minimum initiation interval and measured maximum execution time + a critical instant analysis of the rate monotonic scheduler, showing that all deadlines are met under worst-case conditions)_
### 3.1. ScanKeyTask
The ScanKeyTask function is responsible for reading the key matrix

## 4. Analysis
* _(Include, in this section, pt. 19, 20 & 21 of the *Documentation Specifications*)_

ScanKeyTask: 8430 / 32 = 263 $\mu s$ 

displayUpdateTask: 554261 / 32 = 17 320 $\mu s$ 

sampleISR: 689 / 32 = 21.5 $\mu s$  
