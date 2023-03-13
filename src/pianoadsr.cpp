#include <atomic>


class ADSR{

    volatile std::atomic<uint8_t> state;
    volatile std::atomic<uint8_t> amplitude;

    public:
        ADSR(): state(0), amplitude(0.){}
        void nextState(uint8_t isPressed){

            if(isPressed){
                if(state == 0){
                    if(amplitude >= 64){
                        state = 1;
                    }
                    else{
                        amplitude += 32;
                    }
                }
                else if(state == 1)
                {
                    if(amplitude <= 40){
                        amplitude = 40;
                        state = 3;
                    }
                    else{
                        amplitude -= 1;
                    }
                }
                else if(state == 3){
                    if(amplitude <= 20 | amplitude > 64){
                        amplitude = 0;
                        state = 0;
                    }
                    else{
                        amplitude -= 4;
                    }                   
                }
                 
            }
            else{
                state = 3;
                if(amplitude <= 20 | amplitude > 64){
                    amplitude = 0;
                }
                else{
                    amplitude -= 4;
                }
                
            }
        }
        uint8_t getAmplitude(){
            return  amplitude.load();
        }
        uint8_t getState(){
            return state.load();
        }

};