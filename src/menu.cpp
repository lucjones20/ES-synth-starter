#ifndef MENU_CPP
#define MENU

#include <atomic>
#include <vector>
#include "common_structures.cpp"

class Menu
{
public:
    static void updateMenu(JoyStickState joy, uint8_t selectMode) //1. is it confirm, 2. is it cancel, 3. go up, 4 switch mode, 5 change volume, 6 change octave
    {
        switch(selectMode)
        {
            case 0: 
                controlMode = Normal;
                break;
            case 1:
                if(controlMode != RecordOn)
                {
                    controlMode = RecordOff;
                    if(joy == Up && !newRecord)
                    {
                        recs->push_back(new Recording());
                        recIndex = recs->size()-1;
                        newRecord = true;
                        controlMode = RecordOn;
                    }
                        
                }
                else
                {
                    
                }
                break;
            case 2:
                
        }

    }
private:
    static void decreaseRecIndex()
    {
        if(recIndex.load() != 0)
            recIndex--;
        else if(recs->size() == 0)
            recIndex = 0;
        else
            recIndex = recs->size() - 1;
    }
    static void increaseRecIndex()
    {
        if(recIndex.load() != recs->size() - 1)
            recIndex++;
        else
            recIndex = 0;

    }
    static bool newRecord;
    static std::atomic<Mode> controlMode;
    static std::vector<Recording*>* recs;
    static std::atomic<uint8_t> recIndex;

};

#endif