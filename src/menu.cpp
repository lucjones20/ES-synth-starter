#ifndef MENU_CPP
#define MENU

#include <atomic>
#include <vector>
#include "common_structures.cpp"
#include <STM32FreeRTOS.h>
class Menu
{
private:
    inline static bool newRecord;
    inline static bool onLoop;
    inline static std::atomic<Mode> controlMode;
    inline static std::vector<Recording*>* recs;
    inline static std::atomic<uint8_t> recIndex;
    inline static SemaphoreHandle_t* recordingMutex;
    inline static int prevCounter;
    static void resetVars()
    {
        Menu::newRecord = false;
        Menu::onLoop = false;
    }
    static void decreaseRecIndex()
    {
        if(Menu::recIndex.load() != 0)
            Menu::recIndex--;
        else if(Menu::recs->size() == 0)
            Menu::recIndex = 0;
        else
            Menu::recIndex = recs->size() - 1;
    }
    static void increaseRecIndex()
    {
        if(Menu::recIndex.load() != recs->size() - 1)
            Menu::recIndex++;
        else
            Menu::recIndex = 0;

    }
   


public:
    static void setupMen(std::vector<Recording*>* r, SemaphoreHandle_t* mutex)
    {
        Menu::recs = r;
        Menu::newRecord = false;
        Menu::onLoop = false;
        prevCounter = 0;
        recordingMutex = mutex;
    }
    static void updateMenu(bool yes, bool no, bool extraButt1, bool extraButt2, uint32_t selectMode) //1. is it confirm, 2. is it cancel, 3. go up, 4 switch mode, 5 change volume, 6 change octave
    {
            if(controlMode.load() != Servant)
            {
            if(recs->size() == 0 && selectMode == 2)
                selectMode = 0;
            switch(selectMode)
            {
                case (uint32_t)0: 
                    Menu::controlMode = Normal;
                    break;
                case (uint32_t)1:
                    if(Menu::controlMode.load() == RecordOff)
                    {
                        if(yes && !newRecord)
                        {
                            xSemaphoreTake(*recordingMutex, portMAX_DELAY);
                            Menu::recs->push_back(new Recording());
                            Menu::recIndex = recs->size()-1;
                            xSemaphoreGive(*recordingMutex);
                            Menu::newRecord = true;
                            Menu::controlMode = RecordOn;
                        }
                        else if(yes)
                        {
                            Menu::controlMode = RecordOn;
                        }
                        else
                        {
                            controlMode = RecordOff;
                        }
                    }
                    else if(Menu::controlMode.load() == RecordOn)
                    {
                        if(extraButt1)
                            Menu::controlMode = RecordOff;
                        else if(no)
                        {
                            Menu::newRecord = false;
                            Menu::controlMode = RecordOff;
                        }
                        else
                        {
                            controlMode = RecordOn;
                        }
                    }
                    else
                    {
                        resetVars();
                        Menu::controlMode = RecordOff;
                    }
                    break;
                case (uint32_t)2:
                    if(Menu::controlMode.load() == PlaybackOff)
                    {
                        xSemaphoreTake(*recordingMutex, portMAX_DELAY);
                        if(extraButt1 && prevCounter == 0)
                        {
                            decreaseRecIndex();
                            prevCounter++;
                        }
                        if(extraButt2 && prevCounter == 0 )
                        {
                            increaseRecIndex();
                            prevCounter++;
                        }
                        xSemaphoreGive(*recordingMutex);
                        if(yes)
                            Menu::controlMode = PlaybackOn;
                        if(!extraButt1 && !extraButt2)
                            prevCounter = 0;
                    }
                    else if(Menu::controlMode.load() == PlaybackOn)
                    {
                        if(no)
                            Menu::controlMode = PlaybackOff;
                    }
                    else
                    {
                        resetVars();
                        Menu::controlMode = PlaybackOff;
                    }
                    break;
                default: break;
                    
            }
        }

    }
    static Mode getMode(){return Menu::controlMode.load();}
    static int getRecordIndex(){return recIndex.load();}
    static void setToServant(){controlMode = Servant;}
};
#endif