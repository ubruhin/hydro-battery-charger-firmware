#include "adc.h"
#include "analogin.h"
#include "application.h"
#include "comparator.h"
#include "digitalin.h"
#include "digitalout.h"
#include "display.h"
#include "pwm.h"
#include "spi.h"
#include "system.h"

static const float VDC_R2     = (4.7F * 100.0F) / (4.7F + 100.0F);
static const float VDC_SCALE  = (100.0F + VDC_R2) / VDC_R2;
static const float VGEN_SCALE = (100.0F + 2.2F) / 2.2F;
static const float VBAT_SCALE = (100.0F + 10.0F) / 10.0F;
static const float IBAT_SCALE = 1.0F / 0.1F;

static Pwm* sPwm = nullptr;

static void comparatorCallback() {
  if (sPwm) {
    sPwm->disable();
  }
}

int main(void) {
  // System
  System system;

  // ADC channels
  Adc      adc(ADC1);
  AnalogIn vdc(GPIOA, LL_GPIO_PIN_1, adc, LL_ADC_CHANNEL_1, VDC_SCALE);
  AnalogIn vgen(GPIOA, LL_GPIO_PIN_2, adc, LL_ADC_CHANNEL_2, VGEN_SCALE);
  AnalogIn vbat(GPIOA, LL_GPIO_PIN_3, adc, LL_ADC_CHANNEL_3, VBAT_SCALE);
  AnalogIn ibat(GPIOA, LL_GPIO_PIN_4, adc, LL_ADC_CHANNEL_4, IBAT_SCALE);
  AnalogIn pot(GPIOA, LL_GPIO_PIN_5, adc, LL_ADC_CHANNEL_5, 1.0F / 3.3F);

  // Comparator
  Comparator comparator(COMP1, LL_COMP_INPUT_PLUS_IO1, &comparatorCallback);

  // Charge controller
  Pwm pwm(TIM2, LL_TIM_CHANNEL_CH1);
  sPwm = &pwm;
  DigitalOut pwmOut(GPIOA, LL_GPIO_PIN_15, false, LL_GPIO_AF_5);
  DigitalOut chargeEnable(GPIOA, LL_GPIO_PIN_11, false);

  // Buttons/LEDs
  DigitalIn  button1(GPIOA, LL_GPIO_PIN_0);
  DigitalIn  button2(GPIOA, LL_GPIO_PIN_12);
  DigitalOut ledGreen(GPIOB, LL_GPIO_PIN_0, false);
  DigitalOut ledRed(GPIOB, LL_GPIO_PIN_1, false);

  // SPI
  Spi        spi(SPI1);
  DigitalOut spiSclk(GPIOB, LL_GPIO_PIN_3, true, LL_GPIO_AF_0);
  DigitalOut spiMiso(GPIOB, LL_GPIO_PIN_4, true, LL_GPIO_AF_0);
  DigitalOut spiMosi(GPIOB, LL_GPIO_PIN_5, true, LL_GPIO_AF_0);

  // Display
  DigitalOut displayCs(GPIOA, LL_GPIO_PIN_8, false);
  DigitalOut displayNrst(GPIOA, LL_GPIO_PIN_9, false);
  DigitalOut displayEnable(GPIOA, LL_GPIO_PIN_10, true);
  Display    display(spi, displayCs, displayEnable, displayNrst);

  // Unused pins
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_6, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_7, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_ANALOG);

  // Application
  Application application(system, adc, vgen, vdc, vbat, ibat, chargeEnable, pwm,
                          pot, button1, button2, ledGreen, ledRed, display);
  if (system.GetWokeUpFromWatchdog()) {
    application.runChargeMode();
  } else {
    application.runDisplayMode();
  }

  // this line should actually never be reached... if it is, watchdog will reset
  while (1) {
  }
}
