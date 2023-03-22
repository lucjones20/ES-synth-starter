#ifndef COMMON_TYPES
#define COMMON_TYPES
#include <set>
#include <vector>
#include <stdint.h>



enum Mode{Normal, RecordOn, RecordOff, PlaybackOn, PlaybackOff, Servant};
struct Recording{
  std::vector<std::pair<uint8_t,uint16_t>> keyStrokes;
  std::set<uint8_t> pressed;
  uint32_t lastPressed = 0;
  int curIndex = 0;
};


#endif