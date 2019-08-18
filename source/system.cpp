#include "system.h"

#include "stm32l0xx_ll_cortex.h"
#include "stm32l0xx_ll_pwr.h"
#include "stm32l0xx_ll_rcc.h"
#include "watchdog.h"

System::System() {
  // Enable LSI and watchdog
  LL_RCC_LSI_Enable();
  while (!LL_RCC_LSI_IsReady())
    ;
  Watchdog::enable();

  // Enable GPIO clocks
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

  // Enable SPI clock
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  // Enable ADC clock
  RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

  // Enable TIM2 clock
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
}

#pragma GCC push_options
#pragma GCC optimize("O0")
void        System::delay(uint32_t ms) {
  for (volatile uint32_t i = 0U; i < ms; i++) {
    for (volatile uint32_t k = 0U; k < 130U; ++k)
      ;
  }
}
#pragma GCC pop_options

void System::sleep() {
  __disable_irq();
  LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
  LL_LPM_EnableDeepSleep();
  LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
  LL_PWR_ClearFlag_WU();
  __WFI();
  while (1)
    ;
}
