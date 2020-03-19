#include "rpm_measurement.h"

#include "input_edge_counter.h"
#include "stm32l0xx_ll_cortex.h"

static RpmMeasurement* sInstance = nullptr;

RpmMeasurement::RpmMeasurement(InputEdgeCounter& counter)
  : mCounter(counter), mTimeMs(0U), mRpm(0) {
  SysTick->VAL  = 0U;
  SysTick->LOAD = 4000UL;  // 1ms

  sInstance = this;
}

void RpmMeasurement::enable() {
  mCounter.enable();
  SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
}

void RpmMeasurement::disable() {
  SysTick->CTRL = 0U;
  mCounter.disable();
}

void RpmMeasurement::sysTick() {
  ++mTimeMs;
  if (mTimeMs >= 1000UL) {
    mRpm    = mCounter.getValueAndReset() * 60UL;
    mTimeMs = 0U;
  }
}

extern "C" void SysTick_Handler() {
  if (sInstance) {
    sInstance->sysTick();
  }
}
