#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <main.hpp>
#include <Arduino.h>

double noteFrequencies[12]={
    // C
    double( f / factor / factor / factor / factor / factor / factor / factor / factor / factor / fs),
    // C#
    double( f / factor / factor / factor / factor / factor / factor / factor / factor / fs),
    // D
    double( f / factor / factor / factor / factor / factor / factor / factor / fs),
    // D#
    double( f / factor / factor / factor / factor / factor / factor / fs),
    // E
    double( f / factor / factor / factor / factor / factor / fs),
    // F
    double( f / factor / factor / factor / factor / fs),
    // F#
    double( f / factor / factor / factor / fs),
    // G
    double( f / factor / factor / fs),
    // G#
    double( f / factor / fs),
    // A
    double( f / fs),
    // A#
    double( f * factor / fs),
    // B
    double( f * factor * factor / fs)
};

void initSineArray(SineArray* sineArray){    
    for(int i=0; i<1; i++){
        for(int j=0; j<400; j++){
            sineArray->octave1[i][j] = 2147483648 * sin(( 2 * 3.1415 * noteFrequencies[i] / 8 * j  )) + 2147483648;
        }
        for(int j=0; j<200; j++){
            sineArray->octave2[i][j] = 2147483648 * sin(( 2 * 3.1415 * noteFrequencies[i] / 4 * j  )) + 2147483648;
        }
        for(int j=0; j<100; j++){
            sineArray->octave3[i][j] = 2147483648 * sin(( 2 * 3.1415 * noteFrequencies[i] / 2 * j  )) + 2147483648;
        }
        for(int j=0; j<50; j++){
            sineArray->octave4[i][j] = 2147483648 * sin(( 2 * 3.1415 * noteFrequencies[i] * j  )) + 2147483648;
        }
        for(int j=0; j<25; j++){
            sineArray->octave4[i][j] = 2147483648 * sin(( 2 * 3.1415 * noteFrequencies[i] * 2 * j  )) + 2147483648;
        }
        for(int j=0; j<13; j++){
            sineArray->octave4[i][j] = 2147483648 * sin(( 2 * 3.1415 * noteFrequencies[i] * 4 * j  )) + 2147483648;
        }
        for(int j=0; j<7; j++){
            sineArray->octave4[i][j] = 2147483648 * sin(( 2 * 3.1415 * noteFrequencies[i] * 8 * j  )) + 2147483648;
        }
    }
}