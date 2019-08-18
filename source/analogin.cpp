#include "analogin.h"

#include "adc.h"

AnalogIn::AnalogIn(GPIO_TypeDef* gpio, uint32_t pin, Adc& adc, uint8_t channel,
                   float scale)
  : mAdc(adc), mChannel(channel), mScale(scale) {
  LL_GPIO_InitTypeDef params;
  LL_GPIO_StructInit(&params);
  params.Pin        = pin;
  params.Mode       = LL_GPIO_MODE_ANALOG;
  params.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  params.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  params.Pull       = LL_GPIO_PULL_NO;
  params.Alternate  = LL_GPIO_AF_0;
  LL_GPIO_Init(gpio, &params);
}

float AnalogIn::measure() {
  return mAdc.measureVolts(mChannel) * mScale;
}
