#include <string>

#ifndef MAIN_HPP
#define MAIN_HPP 

#define fs 22000
#define f 440
#define factor 1.05946309436

struct Note {
    std::string name;
    uint32_t stepSize;
};

extern Note stepSizes[12];

struct SineArray{
    uint32_t octave1[12][400];
    uint32_t octave2[12][200];
    uint32_t octave3[12][100];
    uint32_t octave4[12][50];
    uint32_t octave5[12][25];
    uint32_t octave6[12][13];
    uint32_t octave7[12][7];
};

void initSineArray(SineArray* sineArray);

#endif