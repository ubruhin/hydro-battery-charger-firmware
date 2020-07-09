#include "system.h"

#include "stm32l0xx_ll_cortex.h"
#include "stm32l0xx_ll_pwr.h"
#include "stm32l0xx_ll_rcc.h"
#include "watchdog.h"

System::System() : mWokeUpFromWatchdog(false) {
  // Enable LSI and watchdog
  LL_RCC_LSI_Enable();
  while (!LL_RCC_LSI_IsReady())
    ;
  Watchdog::enable();

  // Enable HSI
  LL_RCC_HSI_Enable();
  while (!LL_RCC_HSI_IsReady())
    ;

  // Set flash wait states
  FLASH->ACR |= FLASH_ACR_LATENCY;

  // Enable PLL: 16MHz -> 64MHz -> 32MHz
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLL_MUL_4,
                              LL_RCC_PLL_DIV_2);
  LL_RCC_PLL_Enable();
  while (!LL_RCC_PLL_IsReady())
    ;

  // Use PLL as system clock
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    ;

  // Disable MSI
  LL_RCC_MSI_Disable();

  // Enable GPIO clocks
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
  RCC->IOPENR |= RCC_IOPENR_GPIOCEN;

  // Enable SPI clock
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

  // Enable ADC clock
  RCC->APB2ENR |= RCC_APB2ENR_ADCEN;

  // Enable TIM2 clock
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

  // Enable PWR clock
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;

  // Detect & reset wakeup reason
  mWokeUpFromWatchdog = LL_RCC_IsActiveFlag_IWDGRST();
  LL_RCC_ClearResetFlags();
}

#pragma GCC push_options
#pragma GCC optimize("O0")
void        System::delay(uint32_t ms) {
  for (volatile uint32_t i = 0U; i < ms; i++) {
    for (volatile uint32_t k = 0U; k < 1300U; ++k)
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
