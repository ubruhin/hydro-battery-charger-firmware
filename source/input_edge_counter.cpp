#include "input_edge_counter.h"

static InputEdgeCounter* sInstance = nullptr;

InputEdgeCounter::InputEdgeCounter(uint32_t extiLine)
  : mExtiLine(extiLine), mValue(0U) {
  LL_EXTI_EnableRisingTrig_0_31(mExtiLine);
  LL_EXTI_EnableFallingTrig_0_31(mExtiLine);
  NVIC_SetPriority(EXTI4_15_IRQn, 0);
  sInstance = this;
}

void InputEdgeCounter::enable() {
  LL_EXTI_EnableIT_0_31(mExtiLine);
  NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void InputEdgeCounter::disable() {
  LL_EXTI_DisableIT_0_31(mExtiLine);
  NVIC_DisableIRQ(EXTI4_15_IRQn);
}

uint32_t InputEdgeCounter::getValueAndReset() {
  uint32_t value = mValue;
  mValue         = 0U;
  return value;
}

void InputEdgeCounter::increment() {
  ++mValue;
}

extern "C" void EXTI4_15_IRQHandler() {
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_ALL);
  if (sInstance) {
    sInstance->increment();
  }
}
