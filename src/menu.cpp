#ifndef MENU_CPP
#define MENU

#include <atomic>
#include <vector>
#include "common_structures.cpp"

class Menu
{
private:
    static bool newRecord;
    static bool onLoop;
    static std::atomic<Mode> controlMode;
    static std::vector<Recording*>* recs;
    static std::atomic<uint8_t> recIndex;
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
    static void setupMen(std::vector<Recording*>* r)
    {
        Menu::recs = r;
        Menu::newRecord = false;
        Menu::onLoop = false;
    }
    static void updateMenu(JoyStickState joy, uint8_t selectMode) //1. is it confirm, 2. is it cancel, 3. go up, 4 switch mode, 5 change volume, 6 change octave
    {
        switch(selectMode)
        {
            case 0: 
                Menu::controlMode = Normal;
                break;
            case 1:
                if(Menu::controlMode == RecordOff)
                {
                    if(joy == Up && !newRecord)
                    {
                        Menu::recs->push_back(new Recording());
                        Menu::recIndex = recs->size()-1;
                        Menu::newRecord = true;
                        Menu::controlMode = RecordOn;
                    }
                    else if(joy == Up)
                    {
                        Menu::controlMode = RecordOn;
                    }  
                }
                else if(Menu::controlMode == RecordOn)
                {
                    if(joy == Up)
                        Menu::controlMode = RecordOff;
                    if(joy == Down)
                    {
                        Menu::newRecord = false;
                        Menu::controlMode = RecordOff;
                    }
                }
                else
                {
                    resetVars();
                    Menu::controlMode == RecordOff;
                }
                break;
            case 2:
                if(Menu::controlMode == PlaybackOff)
                {
                    if(joy == Left)
                        Menu::decreaseRecIndex();
                    else if(joy == Right)
                        Menu::increaseRecIndex();
                    else if(joy == Up)
                        Menu::controlMode = PlaybackOn;
                    else if(joy == Down)
                        Menu::onLoop = false;
                }
                if(Menu::controlMode == PlaybackOn)
                {
                    if(joy == Down)
                        Menu::controlMode = PlaybackOff;
                }
                else
                {
                    resetVars();
                    Menu::controlMode = PlaybackOff;
                }

                
        }

    }
    static Mode getMode(){return Menu::controlMode.load();}
};
#endif