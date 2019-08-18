#include "watchdog.h"

#include "stm32l0xx_ll_iwdg.h"

void Watchdog::enable() {
  LL_IWDG_EnableWriteAccess(IWDG);
  LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_256);
  LL_IWDG_SetReloadCounter(IWDG, 0x05A5);
  LL_IWDG_DisableWriteAccess(IWDG);
  LL_IWDG_ReloadCounter(IWDG);
  LL_IWDG_Enable(IWDG);
}

void Watchdog::reset() {
  LL_IWDG_ReloadCounter(IWDG);
}
