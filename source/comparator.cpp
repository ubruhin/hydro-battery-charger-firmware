#include "comparator.h"

static Comparator::Callback sCallback = nullptr;

Comparator::Comparator(COMP_TypeDef* comparator, uint32_t input,
                       Callback callback)
  : mComparator(comparator) {
  sCallback = callback;

  LL_COMP_InitTypeDef params;
  LL_COMP_StructInit(&params);
  params.PowerMode      = LL_COMP_POWERMODE_MEDIUMSPEED;
  params.InputPlus      = input;
  params.InputMinus     = LL_COMP_INPUT_MINUS_VREFINT;
  params.OutputPolarity = LL_COMP_OUTPUTPOL_NONINVERTED;
  LL_COMP_Init(mComparator, &params);
}

extern "C" void ADC_COMP_IRQHandler(void) {
  __disable_irq();
  if (sCallback) {
    sCallback();
  }
  NVIC_SystemReset();
}
