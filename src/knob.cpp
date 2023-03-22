#include <string>
#include <iostream>
#include <atomic>

class Knob{
private:
  std::atomic<uint32_t> state;
  uint32_t lowerBound;
  uint32_t upperBound;
  uint8_t prevBA;
  bool prevUp = true;
  bool prevNormal = false;
  void setCounter(int i)
  {
    if(i == 1 && state.load() < upperBound)
    {
        state++;
    }
    else if(i == -1 && state.load() > lowerBound)
    {
        state--;
    }
  }
public:
  Knob(uint32_t lb, uint32_t ub, uint32_t initCounter)
  {
    lowerBound = lb;
    upperBound = ub;
    state = initCounter;
    prevBA = 0b00;
  }
  void advanceState(uint8_t currentBA)
  {
      switch (prevBA)
      {
      case 0b00:
        switch (currentBA)
        {
          case 0b01:
            prevUp = true;
            prevNormal = true;
            setCounter(1);
            break;

          case 0b11:
            if(prevUp && prevNormal)
              setCounter(1);
            else if(prevNormal)
              setCounter(-1);
            prevNormal = false;
            break;

          default:
            break;
        }
        break;
      case 0b01:
        switch (currentBA)
        {
          case 0b00:
            prevUp = false;
            prevNormal = true;
            setCounter(-1);
            break;

          case 0b10:
            if(prevUp&&prevNormal)
              setCounter(1);
            else if(prevNormal)
              setCounter(-1);
            prevNormal = false;
            break;
      
          default:
            break;
        }
      case 0b10:
        switch (currentBA)
        {
          case 0b11:
            prevUp = false;
            prevNormal = true;
            setCounter(-1);
            break;
          
          case 0b01:
            if(prevUp && prevNormal)
              setCounter(1);
            else if(prevNormal)
              setCounter(-1);
            prevNormal = false;
            break;

          default:
            break;
        }
      case 0b11:
        switch (currentBA)
        {
          case 0b10:
            prevUp = true;
            prevNormal = true;
            setCounter(1);
            break;

          case 0b00:
            if(prevUp && prevNormal)
              setCounter(1);
            else if(prevNormal)
              setCounter(-1);
            prevNormal = false;
            break;
          default:
            break;
        }
      default:
        break;
      }
      prevBA = currentBA;
  }
  void setLimits(uint32_t lower, uint32_t higher)
  {
    lowerBound = lower;
    upperBound = higher;
  }
  uint32_t getCounter(){return state.load();}


};
