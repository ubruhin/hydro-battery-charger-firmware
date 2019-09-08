#include "pwm.h"

Pwm::Pwm(TIM_TypeDef* timer, uint32_t channel)
  : mTimer(timer), mChannel(channel) {
  LL_TIM_InitTypeDef params;
  LL_TIM_StructInit(&params);
  params.Prescaler     = (uint16_t)0x0000;
  params.CounterMode   = LL_TIM_COUNTERMODE_CENTER_UP_DOWN;
  params.Autoreload    = 500UL;
  params.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(mTimer, &params);

  LL_TIM_OC_InitTypeDef ocParams;
  ocParams.OCMode       = LL_TIM_OCMODE_PWM1;
  ocParams.OCState      = LL_TIM_OCSTATE_ENABLE;
  ocParams.OCPolarity   = LL_TIM_OCPOLARITY_HIGH;
  ocParams.CompareValue = 0U;
  LL_TIM_OC_Init(mTimer, mChannel, &ocParams);
}

void Pwm::enable() {
  LL_TIM_EnableCounter(mTimer);
}

void Pwm::disable() {
  setDutyCycleNormalized(0.0F);
  LL_TIM_DisableCounter(mTimer);
}

void Pwm::setDutyCycleNormalized(float value) {
  uint32_t fullscale = LL_TIM_GetAutoReload(mTimer);
  float    ticksF    = fullscale * value;
  uint32_t ticksU    = static_cast<uint32_t>(ticksF);
  if (ticksF < 0.0f) {
    ticksU = 0U;
  } else if (ticksF > (float)fullscale) {
    ticksU = fullscale;
  }
  LL_TIM_OC_SetCompareCH1(mTimer, ticksU);
}
