#include "stm32l0xx_ll_gpio.h"

class DigitalIn {
public:
  DigitalIn()                       = delete;
  DigitalIn(const DigitalIn& other) = delete;
  DigitalIn(GPIO_TypeDef* gpio, uint32_t pin);

  bool read();

private:
  GPIO_TypeDef* mGpio;
  uint32_t      mPinMask;
};
