#include <atomic>


class pianoADSR{

    volatile std::atomic<uint8_t> state;
    volatile std::atomic<uint8_t> amplitude;

    public:
        pianoADSR(): state(0), amplitude(0.){}
        void nextState(uint8_t isPressed, uint8_t previsPressed){

            if(isPressed && !previsPressed){
                amplitude = 64;
                state = 1;
            }
            else if (isPressed && previsPressed)
            {
                if(amplitude <= 10){
                    amplitude = 0;
                }
                else{
                    amplitude -= 2;
                }
            }
            else if (!isPressed && previsPressed)
            {
                state = 3;
            }
            
            else{
                if(amplitude <= 10 || amplitude > 64){
                    amplitude = 0;
                    state = 0;
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