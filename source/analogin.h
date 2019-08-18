#include "stm32l0xx_ll_gpio.h"

class Adc;

class AnalogIn {
public:
  AnalogIn()                      = delete;
  AnalogIn(const AnalogIn& other) = delete;
  AnalogIn(GPIO_TypeDef* gpio, uint32_t pin, Adc& adc, uint8_t channel,
           float scale);

  float measure();

private:
  Adc&    mAdc;
  uint8_t mChannel;
  float   mScale;
};
