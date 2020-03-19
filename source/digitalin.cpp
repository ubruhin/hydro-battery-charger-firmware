#include "digitalin.h"

DigitalIn::DigitalIn(GPIO_TypeDef* gpio, uint32_t pin, uint32_t pull)
  : mGpio(gpio), mPinMask(pin) {
  LL_GPIO_InitTypeDef params;
  LL_GPIO_StructInit(&params);
  params.Pin        = mPinMask;
  params.Mode       = LL_GPIO_MODE_INPUT;
  params.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  params.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  params.Pull       = pull;
  params.Alternate  = LL_GPIO_AF_0;
  LL_GPIO_Init(mGpio, &params);
}

DigitalIn::DigitalIn(GPIO_TypeDef* gpio, uint32_t pin, uint32_t pull,
                     uint32_t alternateFunction)
  : mGpio(gpio), mPinMask(pin) {
  LL_GPIO_InitTypeDef params;
  LL_GPIO_StructInit(&params);
  params.Pin        = mPinMask;
  params.Mode       = LL_GPIO_MODE_ALTERNATE;
  params.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  params.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  params.Pull       = pull;
  params.Alternate  = alternateFunction;
  LL_GPIO_Init(mGpio, &params);
}

bool DigitalIn::read() {
  return (LL_GPIO_ReadInputPort(mGpio) & mPinMask) != 0U;
}
