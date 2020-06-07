#pragma once

#include "stm32l0xx_ll_adc.h"

class Adc {
public:
  Adc()                 = delete;
  Adc(const Adc& other) = delete;
  Adc(ADC_TypeDef* adc);

  void  enable();
  void  disable();
  float measureVolts(uint8_t channel);

private:
  ADC_TypeDef* mAdc;
};
