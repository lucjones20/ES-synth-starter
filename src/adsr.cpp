#include <atomic>


class ADSR{

    volatile std::atomic<uint8_t> state;
    volatile std::atomic<uint8_t> amplitude;

    public:
        ADSR(): state(0), amplitude(0.){}
        void nextState(uint8_t isPressed){

            if(isPressed){
                if(state == 0){
                    if(amplitude >= 8){
                        state = 1;
                    }
                    else{
                        amplitude += 1;
                    }
                }
                else if(state == 1)
                {
                    if(amplitude <= 8){
                        amplitude = 6;
                        state = 2;
                    }
                    else{
                        amplitude -= 1;
                    }
                }
                else if(state == 3){
                    state = 0;
                }
                 
            }
            else{
                state = 3;
                if(amplitude <= 0){
                    amplitude = 0;
                }
                else{
                    amplitude -= 1;
                }
                
            }
        }
        double getAmplitude(){
            return  amplitude.load();
        }
        uint8_t getState(){
            return state.load();
        }

};