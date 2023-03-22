DOCUMENTATION
=============================

Table of Contents
-----------------------------

1. [Introduction](#1-introduction)  
2. [Features](#2-features)  
    2.1. [Core Features](#21-core-features)      
    2.2. [Advanced Features](#22-advanced-features)  
3. [Tasks](#3-tasks)  
    3.1. [scanKeyTask](#31-scankeytask)  
    3.2. [displayUpdateTask](#32-displayupdatetask)    
    3.3. [sampleISR](#33-sampleisr)    
    3.4. [CAN_TX_Task](#34-can\_tx\_task)  
    3.5. [CAN_RX_Task](#35-can\_rx\_task)  
4. [Analysis](#4-analysis)   
5. [References](#5-references)  


<!-- [0.0. Test Test](#00-test-test)
### 0.0. Test Test -->

&nbsp;  
## 1. Introduction 
<!-- &nbsp;   -->

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

**Group members:** Zsombor Klapper, Luc Jones, Bastien Baluyot, Abdal Al-Kilany.

&nbsp;  

## 2. Features
<!-- &nbsp;   -->

### 2.1. Core Features
&nbsp;  
- The synthesizer plays the correct music note with a sawtooth wave when the corresponding key is pressed without any delay between the key press and the tone starting.
- There are 8 different volume setting which can be controlled and adjusted with a knob.
- The OLED display shows the current notes being played and the current volume setting, amongst other additional information (see Menu in [Advanced Features](#22-advanced-features)).
    - The OLED display refreshes and the LED LD3 (on the MCU module) toggles every 100ms.
- The synthesiser can be configured as a sender or reciever at compile time.
    - If configured as a sender, the synthesiser sends the appropriate note(s) when a key is pressed/released as a message via the CAN bus.
    - If configured as receiver, the synthesiser plays/stops playing the appropriate note(s) after receiving the message.
&nbsp;  



### 2.2. Advanced Features 
_(add Figures/Videos later when revising...)_

&nbsp;  
- a. Polyphony
    - With the default/base implementation of phaseAcc and currentStepSize, only one note was able to be played at any given time and if multiple notes were played at the same time, the right-most note would be played.

    - By changing the phaseAcc variable to a uint32_t array, phaseAccArray, all currently pressed keys are accounted for. phaseAccArray has size 96 since there are 12 keys per octave and 8 octaves.

    - Vout is the total sum corresponding to each of the pressed keys and the resulting sound achieves polyphony.

    - **Overloading safety measures**: After each key's release, the elements are erased from the array and Vout is saturated to be in the range of [-127, 127] so that the Vout written using analogWrite is saturated to [1,255] and the voltage range is limited safely to 0-3.3V.

    - >**[DEMO VIDEO CLIP FOR POLYPHONY]**

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

        <p align="center">
            <img src="Images/ADSRgraph.png"  width=75% height=75%>
        </p>

    - The ADSR Envelope feature was implemented with a state machine that reduces the max volume of the sound output according to the phase and stage/progression in the phase.

        <p align="center">
            <img src="Images/ADSRStateMachine.png"  width=75% height=75%>
        </p>

&nbsp;  
- d. Additional Waveforms (Sine, Triangle, Sqaure)
    - In addition to the core sawtooth waveform, sine, triangle and square waveforms were implemented. The waveform can be selected and toggled/cycled through the use of Knob X.   

    - Sine: 
    >- **[GRAPH OF VOUT VS PHASE SHOWING RANGE, PERIOD AND RESOLUTION OF SINE WAVE]**
    <p align = "center">
        <img src="Images/Sine.jpg">
    </p>
    In order to play a sound wave, we cannot compute the sine in real time because it takes too long. The solution to this is to compute a set of discrete values for all of the sine waves of different frequencies and store those precomputed values into an array. To minimise the data needed we only store one period of each sine wave. To play all the octaves, we need to store 84 sine waves. Using the sampling frequency, we determined the minimum number of samples need for each octave: octave 1 needs 400 values and the number of values halves every octave. The number of samples per second (shown with delta, the time between 2 samples, which stays constant as the frequency changes) does not actually change between the octaves, but the time it takes for a period to complete shrinks as the frequency increases, allowing us to decrease the number of samples needed to accurately play a sine wave.

    - Triangle:
    >- **[GRAPH OF VOUT VS PHASE SHOWING RANGE, PERIOD AND RESOLUTION OF TRIANGLE WAVE]**
    <p align = "center">
        <img src="Images/tringale.jpeg">
    </p>

    The triangle waveform is similar to the sawtooth in that it increases and decreases by a fixed stepsize. The step size for the triangle wave is twice the frequency of the sawtooth wave because in the time the sawtooth increases, the triangle has to increase then decrease .Hence the step size needs to be double so that the value of the signal is 0 at the correct time.

    - Square: 
    >- **[GRAPH OF VOUT VS PHASE SHOWING RANGE AND PERIOD OF SQUARE WAVE]**
    <p align = "center">
        <img src = "Images/square.jpeg">
    </p>

    The square wave is a very basic waveform which is either high or low. The way it was implemented is that when phaseAcc is smaller than half of the max int, Vout is set to 0 and when phaseAcc is higher than half the max int, Vout is set to 127.


    &nbsp;  
    - >**[DEMO VIDEO CLIP FOR EITHER EACH WAVEFORM IN ACTION OR EACH OF THE INSTRUMENTS]**

&nbsp;  
- e. Additional Instruments
    - Following the implementation of additional waveforms and ADSR Envelope modulation for the synthesis of music, the synthesis of different, additional instruments were made possible. Each instrument corresponds to a waveform and an ADSR ratio, as shown in the table [[1](#5-references)].

        | Instrument | A | D | S | R | Waveform |
        | :-------: | :-------: | :-------: | :-------: | :-------: | :-------: |
        | Piano | 0 | 9 | 0 | 9 | Sawtooth |
        | Flute | 6 | 0 | 8 | 0 | Square |
        | Violin | 10 | 8 | 10 | 9 | Square |
        | Cello | 0 | 9 | 0 | 0 | Square |
    
    - The most basic sound, the sound of a piano, uses the default sawtooth waveform and inherently has no Attack/Sustain phase. Therefore, the core specification is still satisfied since there is no perceptible delay between initially pressing the key and the volume reaching maximum.
    


&nbsp;  
- f. Recording & Playback
    - A record feature has been implemented, to help the user make more complex plays. The record & playback feature provides the user with ability to record every keystroke supporting multiple tracks (can record multiple tracks) and can later play them back while having the ability to play on top of it. 

    &nbsp; 
    - Recording: The records are stored in a Recording struct, which logs the key presses and releases and the interval between them. It records both incoming messages and outgoing messages and basically stores said messages. Each record structure is stored as a pointer in a std::vector which allows for multiple tracks to be stored. 

    &nbsp;  
    - Playback: The recorded keys are played back using the currently selected waveform on the keyboard. It works the same way as if extra incoming messages from other boards would be interpreted onto the board, thus allowing extra keypresses to be played on top of it. As a result, one can produce complex plays which usually would require multiple people. 
    
    

    - >**[DEMO VIDEO CLIP FOR RECORDING & PLAYBACK]**

&nbsp;  
- g. Menu 

    - The menu allows for interaction and is displayed on the OLED display. The following information/settings are displayed on the menu:
        - Note(s) being played

        - Waveform (Sawtooth/Triangle/Sine/Square)
        - Octave (0-7)
        - Volume level (0-8) 
        - Recording (On/Off) or Playback (On/Off) setting

    &nbsp;  
    - >**[PICTURE OF THE MENU]**

    - >**[PICTURE OF STATE MACHINE OF THE MENU...]**

    &nbsp;
    - Record & Playback controls: For the contols it uses knob presses in the following way: Knob0 - Start, Knob1-Cancel, Knob2-Stop/Decrease, Knob3-Increase. Firstly one must select the RecordOff mode, then by pressing start it will capture the inputs. One can pause it using the stop button or can finalise it and save it with the cancel button. After at least one recording has been recording PlaybackOff will be available for choosing. The user can select which track he wants to play with the decrease/increase buttons and can start the playback with the start button. The playback can be stoped at any moment with the cancel button and then can be resumed with the press of the start button again.

    &nbsp;  
    - _(Does the menu display any information regarding whether there are other keyboard modules connected? e.g. East/West...)_

    - >**[DEMO VIDEO CLIP OF NAVIGATION OF MENU]**

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

> - Mention how we could increase the frequency, allowing for better reading of the knob states (they are not as well-functioning since the detents are not read as properly due to intermediate states...). Just acknowledge this consideration..

&nbsp;  
### 3.2. DisplayUpdateTask
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_

&nbsp;  
### 3.3. SampleISR
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_

&nbsp;  
### 3.4. CAN_TX_Task
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_

&nbsp;  
### 3.5. CAN_RX_Task
_//Description & Implementation/Thread/Interrupt_: 

- Theoretical Minimum Initiation Interval:
- Measured Maximum Execution Time:
- Critical instant analysis of the rate monotic scheduler: _(showing that all deadlines are met under worst-case conditions)_



&nbsp;  
## 4. Analysis
>* _(Include, in this section, pt. 19, 20 & 21 of the *Documentation Specifications*)_
>* _(Probably re-do these tests at the end...)_
>
>* _(Also mention: The parts of the code containing "compile-time options" that allowed us to measure the execution time of each task (pt. 14 of *Non-functional specifications*))_

&nbsp;
### 4.1. Shared Resources 
* recording vector: Used to store each track and used by scanKeyTask (in menu.cpp with pointer reference as well), displayTask, sendTask and recieveTask. It is protected by the recordMutex semaphore.
* notesPlayed: used by displayTask and scanKeyTask and it is protected by keyArrayMutex semaphore.
* keyPress: used by recieveTask and displayTask and it is protected by keyArrayMutex semaphore.
* amplitudeAmp, amplitudeState, currentStepMap: used by recieveTask, scanKeyTask and sampleISR interrupt. It was a bit more challanging to protect this but achieved thread safety by using an atomic bool and compare swaps. It has two different usege:
    * One with an if(compare and swap) which tries to acces it, if it is not available it just skips it.
    * One with a while(compare and swap) which blocks the thread until available -> important for message recieves 
* queue for outgoing message and incomming message: protected by CAN_TX_Semaphore as detailed in the lab tutorials
* Knob and menu parameters: many parameters from these classes can be accessed from multiple threads at any time ensured by using atomic variables with atomic operations.
 
&nbsp;
### 4.2. Deadlocks
 Our implementation according to our analysis is completely deadlock free. The only interdependent mutex usage is using recordingMutex while inside the critical section of keyArrayMutex in scanKey task. However since this is our only interdependent mutex usage it is ensured that even if one of the mutexes are taken they will be freed without requiring the other mutex. 


&nbsp;
### 4.3. Timing Analysis  

* ScanKeyTask: 8430 / 32 = 263 $\mu s$ 

* displayUpdateTask: 554261 / 32 = 17 320 $\mu s$ 

* sampleISR: 689 / 32 = 21.5 $\mu s$  

### 4.4. CPU Utilisation

- ...


&nbsp;  
## 5. References
[1] - F. W. Wibowo, “The Detection of Signal on Digital Audio Synthesizer Based-On Propeller,” _Advanced Science Letters_, vol. 23, no. 6, pp. 5472–5475, Jun. 2017, doi: https://doi.org/10.1166/asl.2017.7402.

[def]: #1test
