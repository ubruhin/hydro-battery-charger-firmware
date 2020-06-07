#pragma once

#include "stm32l0xx_ll_gpio.h"

class DigitalIn {
public:
  DigitalIn()                       = delete;
  DigitalIn(const DigitalIn& other) = delete;
  DigitalIn(GPIO_TypeDef* gpio, uint32_t pin, uint32_t pull, bool invert);
  DigitalIn(GPIO_TypeDef* gpio, uint32_t pin, uint32_t pull,
            uint32_t alternateFuntion);

  bool read();

private:
  GPIO_TypeDef* mGpio;
  uint32_t      mPinMask;
  bool          mInvert;
};
