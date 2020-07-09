#include "adc.h"
#include "analogin.h"
#include "application.h"
#include "comparator.h"
#include "digitalin.h"
#include "digitalout.h"
#include "input_edge_counter.h"
#include "pwm.h"
#include "rpm_measurement.h"
#include "spi.h"
#include "spi_display.h"
#include "st7066u_display.h"
#include "system.h"
#include "valve.h"

#if (PCB_VERSION != 3) && (PCB_VERSION != 4)
#error "Unknown PCB Version!"
#endif

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

  // RPM measurement
  DigitalIn rpmMeasurementPin(GPIOA, LL_GPIO_PIN_6, LL_GPIO_PULL_DOWN, false);
  InputEdgeCounter rpmEdgeCounter(LL_EXTI_LINE_6);
  RpmMeasurement   rpmMeasurement(rpmEdgeCounter);

  // Charge controller
  Pwm pwm(TIM2, LL_TIM_CHANNEL_CH1);
  sPwm = &pwm;
  DigitalOut pwmOut(GPIOA, LL_GPIO_PIN_15, false, LL_GPIO_AF_5);
  DigitalOut chargeEnable(GPIOB, LL_GPIO_PIN_7, false);

  // Buttons/LEDs
  DigitalIn  button1(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_DOWN, false);
  DigitalIn  button2(GPIOB, LL_GPIO_PIN_6, LL_GPIO_PULL_DOWN, false);
  DigitalOut powerLed(GPIOB, LL_GPIO_PIN_4, false);   // green
  DigitalOut statusLed(GPIOB, LL_GPIO_PIN_5, false);  // yellow

  // Display
  DigitalOut displayPower(GPIOB, LL_GPIO_PIN_0, true);
  DigitalOut displayRS(GPIOB, LL_GPIO_PIN_1, false);
  DigitalOut displayEnable(GPIOA, LL_GPIO_PIN_9, false);
  DigitalOut displayDB4(GPIOA, LL_GPIO_PIN_10, false);
  DigitalOut displayDB5(GPIOA, LL_GPIO_PIN_11, false);
  DigitalOut displayDB6(GPIOA, LL_GPIO_PIN_12, false);
#if PCB_VERSION >= 4
  DigitalOut displayDB7(GPIOA, LL_GPIO_PIN_8, false);
  DigitalOut displayBacklight(GPIOB, LL_GPIO_PIN_3, false);
#else
  DigitalOut displayDB7(GPIOB, LL_GPIO_PIN_3, false);
  DigitalOut displayBacklight(GPIOA, LL_GPIO_PIN_7, false);
  DigitalOut displayRW(GPIOA, LL_GPIO_PIN_8, false);
#endif
  ST7066UDisplay display(displayPower, displayRS, displayEnable, displayDB4,
                         displayDB5, displayDB6, displayDB7, displayBacklight);

  // Switch
#if PCB_VERSION >= 4
  DigitalIn  autoStartSwitch(GPIOA, LL_GPIO_PIN_7, LL_GPIO_PULL_UP, true);
#endif

  // Valve
#if PCB_VERSION >= 4
  DigitalOut valvePower(GPIOC, LL_GPIO_PIN_14, false);
  DigitalOut valveOpen(GPIOC, LL_GPIO_PIN_15, false);
  Valve      valve(valvePower, valveOpen);
#endif

  // Unused pins
#if PCB_VERSION == 3
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_ANALOG);
#endif

  // Application
  Application application(system, adc, vgen, vdc, vbat, ibat, rpmMeasurement,
                          chargeEnable, pwm, pot, button1, button2, powerLed,
                          statusLed, display);
  const bool  displayMode = (!system.getWokeUpFromWatchdog()) || button1.read();
  application.run(displayMode);

  // this line should actually never be reached... if it is, watchdog will reset
  while (1) {
  }
}
