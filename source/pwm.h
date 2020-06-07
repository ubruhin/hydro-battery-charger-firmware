#pragma once

#include "stm32l0xx_ll_tim.h"

class Pwm {
public:
  Pwm()                 = delete;
  Pwm(const Pwm& other) = delete;
  Pwm(TIM_TypeDef* timer, uint32_t channel);

  void enable();
  void disable();
  void setDutyCycleNormalized(float value);

private:
  TIM_TypeDef* mTimer;
  uint32_t     mChannel;
};
