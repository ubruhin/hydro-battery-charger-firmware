#include "adc.h"
#include "analogin.h"
#include "application.h"
#include "comparator.h"
#include "digitalin.h"
#include "digitalout.h"
#include "input_edge_counter.h"
#include "pwm.h"
#include "rpm_measurement.h"
#include "st7066u_display.h"
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

  // RPM measurement
  DigitalIn        rpmMeasurementPin(GPIOA, LL_GPIO_PIN_6, LL_GPIO_PULL_DOWN);
  InputEdgeCounter rpmEdgeCounter(LL_EXTI_LINE_6);
  RpmMeasurement   rpmMeasurement(rpmEdgeCounter);

  // Charge controller
  Pwm pwm(TIM2, LL_TIM_CHANNEL_CH1);
  sPwm = &pwm;
  DigitalOut pwmOut(GPIOA, LL_GPIO_PIN_15, false, LL_GPIO_AF_5);
  DigitalOut chargeEnable(GPIOB, LL_GPIO_PIN_7, false);

  // Buttons/LEDs
  DigitalIn  button1(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_DOWN);
  DigitalIn  button2(GPIOB, LL_GPIO_PIN_6, LL_GPIO_PULL_DOWN);
  DigitalOut ledGreen(GPIOB, LL_GPIO_PIN_4, false);
  DigitalOut ledYellow(GPIOB, LL_GPIO_PIN_5, false);

  // Display
  DigitalOut     displayPower(GPIOB, LL_GPIO_PIN_0, true);
  DigitalOut     displayRS(GPIOB, LL_GPIO_PIN_1, false);
  DigitalOut     displayRW(GPIOA, LL_GPIO_PIN_8, false);
  DigitalOut     displayEnable(GPIOA, LL_GPIO_PIN_9, false);
  DigitalOut     displayDB4(GPIOA, LL_GPIO_PIN_10, false);
  DigitalOut     displayDB5(GPIOA, LL_GPIO_PIN_11, false);
  DigitalOut     displayDB6(GPIOA, LL_GPIO_PIN_12, false);
  DigitalOut     displayDB7(GPIOB, LL_GPIO_PIN_3, false);
  DigitalOut     displayBacklight(GPIOA, LL_GPIO_PIN_7, false);
  ST7066UDisplay display(displayPower, displayRS, displayRW, displayEnable,
                         displayDB4, displayDB5, displayDB6, displayDB7,
                         displayBacklight);

  // Unused pins
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_14, LL_GPIO_MODE_ANALOG);
  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_15, LL_GPIO_MODE_ANALOG);

  // Application
  Application application(system, adc, vgen, vdc, vbat, ibat, rpmMeasurement,
                          chargeEnable, pwm, pot, button1, button2, ledGreen,
                          ledYellow, display);
  if (system.GetWokeUpFromWatchdog() && (button1.read() == false)) {
    application.runChargeMode();
  } else {
    application.runDisplayMode();
  }

  // this line should actually never be reached... if it is, watchdog will reset
  while (1) {
  }
}
