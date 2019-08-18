#include "application.h"

#include "adc.h"
#include "analogin.h"
#include "digitalin.h"
#include "digitalout.h"
#include "display.h"
#include "pwm.h"
#include "system.h"
#include "watchdog.h"

// PWM parameters
static const float PWM_DUTY_CYCLE_STEPS = 0.01F;
static const float PWM_DUTY_CYCLE_MIN   = 0.0F;
static const float PWM_DUTY_CYCLE_MAX   = 0.99F;

// start charging parameters
static const float START_VGEN_MIN      = 25.0F;  // [V]
static const float START_VDC_MAX       = 3.0F;   // [V]
static const float START_VBAT_MIN      = 23.0F;  // [V]
static const float START_VBAT_MAX      = 27.5F;  // [V]
static const float START_PWM_DUTYCYCLE = 0.01F;  // [1]

// charging parameters
static const float CHARGE_IBAT_MIN = 0.05F;  // [A]
static const float CHARGE_VDC_MIN  = 23.0F;  // [V]
static const float CHARGE_VBAT_MAX = 27.5F;  // [V]
static const float CHARGE_PWM_STEP = 0.01F;  // [1]
static const float CHARGE_PWM_MIN  = 0.05F;  // [1]

Application::Application(System& system, Adc& adc, AnalogIn& vgen,
                         AnalogIn& vdc, AnalogIn& vbat, AnalogIn& ibat,
                         DigitalOut& chargeEnable, Pwm& pwm, AnalogIn& pot,
                         DigitalIn& button1, DigitalIn& button2,
                         DigitalOut& ledGreen, DigitalOut& ledRed,
                         Display& display)
  : mSystem(system),
    mAdc(adc),
    mVgen(vgen),
    mVdc(vdc),
    mVbat(vbat),
    mIbat(ibat),
    mChargeEnable(chargeEnable),
    mPwm(pwm),
    mPotentiometer(pot),
    mButton1(button1),
    mButton2(button2),
    mLedGreen(ledGreen),
    mLedRed(ledRed),
    mDisplay(display),
    mMeasuredVgen(0.0F),
    mMeasuredVdc(0.0F),
    mMeasuredVbat(0.0F),
    mMeasuredIbat(0.0F),
    mMeasuredPotentiometer(0.0F),
    mPwmDutyCycle(0.0F) {
}

void Application::process() {
  mLedGreen.setHigh();

  mAdc.enable();
  Measure();

  // update display for 10s
  if (mButton1.read() == false) {
    mDisplay.switchOn();
    for (int i = 0; i < 20; i++) {
      Measure();
      UpdateDisplay("OFF");
      mSystem.delay(500UL);
      Watchdog::reset();
    }
    mDisplay.switchOff();
  }

  // check voltages
  if ((mMeasuredVgen > START_VGEN_MIN) && (mMeasuredVdc < START_VDC_MAX) &&
      (mMeasuredVbat > START_VBAT_MIN) && (mMeasuredVbat < START_VBAT_MAX)) {
    // start charging
    mDisplay.switchOn();
    mChargeEnable.setHigh();
    mPwm.enable();
    for (int i = 0; i < 20; i++) {
      Measure();
      SetPwmDutyCycle(START_PWM_DUTYCYCLE);
      UpdateDisplay("Starting");
      mSystem.delay(500UL);
      Watchdog::reset();
    }

    // charge CC
    do {
      Measure();
      SetPwmDutyCycle(mMeasuredPotentiometer);
      UpdateDisplay("Charge 1/2");
      mSystem.delay(500UL);
      Watchdog::reset();
    } while ((mMeasuredIbat > CHARGE_IBAT_MIN) &&
             (mMeasuredVdc > CHARGE_VDC_MIN) &&
             (mMeasuredVbat < CHARGE_VBAT_MAX));

    // charge CV
    do {
      mLedRed.toggle();
      Measure();
      if (mMeasuredVbat > CHARGE_VBAT_MAX) {
        SetPwmDutyCycle(mPwmDutyCycle - CHARGE_PWM_STEP);
      }
      UpdateDisplay("Charge 2/2");
      mSystem.delay(500UL);
      Watchdog::reset();
    } while ((mMeasuredIbat > CHARGE_IBAT_MIN) &&
             (mMeasuredVdc > CHARGE_VDC_MIN) &&
             (mPwmDutyCycle > CHARGE_PWM_MIN));

    // stop charging
    mPwm.disable();
    System::delay(500UL);  // avoid overvoltage due to energy in coil
    mChargeEnable.setLow();
    mDisplay.switchOff();
  }

  mAdc.disable();
  mLedGreen.setLow();
  mSystem.sleep();
}

void Application::Measure() {
  mMeasuredVgen          = mVgen.measure();
  mMeasuredVdc           = mVdc.measure();
  mMeasuredVbat          = mVbat.measure();
  mMeasuredIbat          = mIbat.measure();
  mMeasuredPotentiometer = mPotentiometer.measure();
}

void Application::SetPwmDutyCycle(float value) {
  if (mPwmDutyCycle < (value - PWM_DUTY_CYCLE_STEPS)) {
    mPwmDutyCycle += PWM_DUTY_CYCLE_STEPS;
  } else if (mPwmDutyCycle > (value + PWM_DUTY_CYCLE_STEPS)) {
    mPwmDutyCycle -= PWM_DUTY_CYCLE_STEPS;
  } else {
    mPwmDutyCycle = value;
  }

  if (!(mPwmDutyCycle >= PWM_DUTY_CYCLE_MIN)) {
    mPwmDutyCycle = PWM_DUTY_CYCLE_MIN;
  }
  if (!(mPwmDutyCycle <= PWM_DUTY_CYCLE_MAX)) {
    mPwmDutyCycle = PWM_DUTY_CYCLE_MAX;
  }

  mPwm.setDutyCycleNormalized(mPwmDutyCycle);
}

void Application::UpdateDisplay(const char* msg) {
  mDisplay.clear();
  mDisplay.print(0, 0, "%3.0fV %4.1fV", mMeasuredVgen, mMeasuredVbat);
  mDisplay.print(1, 0, "%3.0f%% %4.1fV", mPwmDutyCycle * 100.0F, mMeasuredVdc);
  mDisplay.print(2, 0, "%4.2fA %3.0fW", mMeasuredIbat,
                 mMeasuredVbat * mMeasuredIbat);
  mDisplay.print(3, 0, msg);
}
