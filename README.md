DOCUMENTATION
=============================

Table of Contents
-----------------------------

1. [Introduction](#introduction)
2. [Features](#features)    
    2.1. [Core Features](#corefeatures)     
    2.2. [Advanced Features](#advancedfeatures) 
3. [Tasks](#tasks)  
    3.1. [scanKeyTask](#scankeytask)  
    3.2. [displayUpdateTask](#displayupdatetask)    
    3.3. [sampleISR](#sampleISR)    
    3.3. [CAN_TX_Task](#cantxtask)  
    3.4. [CAN_RX_Task](#canrxtask) 
 4. [Analysis](#analysis)   
 5. [References](#references)

&nbsp;  
## 1. Introduction 
&nbsp;  

This project surrounds the embedded software used to program a ST NUCLEO-L432KC microcontroller to control a music synthesiser. Several real-time programming and system analysis techniques were utilised to enable the successful implementation of various different features for the keyboard embedded system, outlined in this documentation.

&nbsp;  

> **Technical specification:** The microntroller contains a STM32L432KCU6U processor, which has an Arm Cortex-M4 core. Each keyboard module covers an octave such that the inputs are 12 keys, 4 control knobs and a joystick. The outputs are 2 channels of audio and an OLED display. Multiple keyboard modules are also able to be connected and stacked together through communication via a CAN bus.
>
> Existing resources used:
>
>- Libraries: STM32duino FreeRTOS and U8g2 Display Driver Libraries
>- Frameworks: STM32duino 
>- Hardware Abstraction Layers (HALs): CMSIS/STM32Cube   

&nbsp;  



This project was completed by Electrical Engineering students from Imperial College London for the 2nd Coursework of the ELEC60013 - Embedded Systems module.

Group members: Zsombor Klapper, Luc Jones, Bastien Baluyot, Abdal Al-Kilany.

&nbsp;  

## 2. Features
&nbsp;  

### 2.1. Core features
&nbsp;  
- The synthesizer plays the correct music note with a sawtooth wave and without any delay
- There are 8 different volumes which you can control with a knob
- OLED display shows the current notes being played and the current volume 
- OLED display refreshes every 100ms and the LD3 LED toggles every 100ms
- The synthesizer can be configured as a sender or reciever at compile time
- The synthesizer will play the sounds if it is a reciever
- The synthesizer will send the note if it is a sender  
&nbsp;  



### 2.2. Advanced features _(add Figures/Videos later when revising...)_

&nbsp;  
- a. Polyphony
    - With the default/base implementation of phaseAcc and currentStepSize, only one note was able to be played at any given time and if multiple notes were played at the same time, the right-most note would be played.

    - By changing the phaseAcc variable to a uint32_t array, phaseAccArray, all currently pressed keys are accounted for. phaseAccArray has size 96 since there are 12 keys per octave and 8 octaves.

    - Vout is the total sum corresponding to each of the pressed keys and the resulting sound achieves polyphony.

    - **Overloading safety measures**: After each key's release, the elements are erased from the array and Vout is saturated to be in the range of [-127, 127] so that the Vout written using analogWrite is saturated to [1,255] and the voltage range is limited safely to 0-3.3V.


&nbsp;  
- b. Octave Knob
    - In addition to the volume knob, Knob X was programmed to control the octave that the corresponding board/module plays in, with the octave ranging from 0-7.

&nbsp;  
- c. ADSR Envelopes
    - In order to provide further control to the sound production of the synthesiser, ADSR envelopes were implemented to modulate the evolution/progression of the sound. The four stages are as follows:

        - A - Attack: Once a key is pressed, the Attack phase controls the speed that the sound reaches full/max volume. It provides a delay factor that can be controlled.

        - D - Decay: After reaching max volume, the Decay phase decreases the volume from full/max to a lower level, which is sustained in the next phase. 

        - S - Sustain: In the Sustain phase, the current volume is maintained for a controlled period of time.

        - R - Release: Finally, the current volume is decreased back to 0/minimum in the Release phase. This is usually a non-linear decay and is implemented as such.

    - The ADSR Envelope feature was implemented with a state machine that reduces the max volume of the sound output according to the phase and stage/progression in the phase.

&nbsp;  
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

&nbsp;  
- e. Other instruments
    - ...

        | Instrument | A | D | S | R | Waveform |
        | :----: | :----: | :----: | :----: | :----: | :----: |
        | Flute | 6 | 0 | 8 | 0 | Square |
        | Violin | 10 | 8 | 10 | 9 | Square |
        | Cello | 0 | 9 | 0 | 0 | Square |
        | Piano | 0 | 9 | 0 | 9 | Sawtooth |

    - each instruments..... 
        - *the piano has no Attack phase so still satisfies the "no perceptible delay" part of the core specs...

&nbsp;  
- f. Recording & Playback
    - ...

&nbsp;  
&nbsp;  
>* _(*Not implemented/Not implemented yet)_
>- _*Joystick_
>    - *...
>- _*Stero sound with multiple boards' speakers_
>    - *...

&nbsp;  
## 3. Tasks
>* _(For each task, include: method of implementation, thread or interrupt + theoretical minimum initiation interval and measured maximum execution time + a critical instant analysis of the rate monotonic scheduler, showing that all deadlines are met under worst-case conditions)_

&nbsp;  
### 3.1. ScanKeyTask
_//Description & Implementation/Thread/Interrupt_: The ScanKeyTask function is responsible for reading the key matrix...

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_

### 3.2. DisplayUpdateTask
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_

### 3.3. SampleISR
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_

### 3.4. CAN_TX_Task
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_

### 3.5. CAN_RX_Task
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_




## 4. Analysis
* _(Include, in this section, pt. 19, 20 & 21 of the *Documentation Specifications*)_
* _(Probably re-do these tests at the end...)_

* _(Also mention: The parts of the code containing "compile-time options" that allowed us to measure the execution time of each task (pt. 14 of *Non-functional specifications*))_

ScanKeyTask: 8430 / 32 = 263 $\mu s$ 

displayUpdateTask: 554261 / 32 = 17 320 $\mu s$ 

sampleISR: 689 / 32 = 21.5 $\mu s$  


## 5. References
[1] - F. W. Wibowo, “The Detection of Signal on Digital Audio Synthesizer Based-On Propeller,” _Advanced Science Letters_, vol. 23, no. 6, pp. 5472–5475, Jun. 2017, doi: https://doi.org/10.1166/asl.2017.7402.
‌

