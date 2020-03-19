#include "stm32l0xx_ll_exti.h"

class InputEdgeCounter {
public:
  InputEdgeCounter()                              = delete;
  InputEdgeCounter(const InputEdgeCounter& other) = delete;
  InputEdgeCounter(uint32_t extiLine);

  void     enable();
  void     disable();
  void     increment();
  uint32_t getValueAndReset();

private:
  uint32_t          mExtiLine;
  volatile uint32_t mValue;
};
