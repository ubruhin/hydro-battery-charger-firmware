#include "stm32l0xx_ll_tim.h"

class InputEdgeCounter;

class RpmMeasurement {
public:
  RpmMeasurement()                            = delete;
  RpmMeasurement(const RpmMeasurement& other) = delete;
  RpmMeasurement(InputEdgeCounter& counter);

  void     enable();
  void     disable();
  void     sysTick();
  uint32_t getRpm() const { return mRpm; }

private:
  InputEdgeCounter& mCounter;
  volatile uint32_t mTimeMs;
  uint32_t          mRpm;
};
