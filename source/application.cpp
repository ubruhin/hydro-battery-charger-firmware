#include "application.h"

#include "adc.h"
#include "analogin.h"
#include "digitalin.h"
#include "digitalout.h"
#include "display.h"
#include "pwm.h"
#include "rpm_measurement.h"
#include "system.h"
#include "watchdog.h"

// PWM parameters
static const float PWM_DUTY_CYCLE_STEPS = 0.01F;  // [1]
static const float PWM_DUTY_CYCLE_MIN   = 0.05F;  // [1]
static const float PWM_DUTY_CYCLE_MAX   = 0.99F;  // [1]

// start charging parameters
static const float START_VGEN_MIN      = 30.0F;  // [V]
static const float START_VDC_MAX       = 3.0F;   // [V]
static const float START_VBAT_MIN      = 22.0F;  // [V]
static const float START_VBAT_MAX      = 27.0F;  // [V]
static const float START_PWM_DUTYCYCLE = 0.2F;   // [1]

// charging parameters
static const float CHARGE_VDC_MIN            = 22.0F;  // [V]
static const float CHARGE_IBAT_MIN           = 0.1F;   // [A]
static const float CHARGE_VBAT_MIN           = 27.2F;  // [V]
static const float CHARGE_VBAT_MAX           = 27.5F;  // [V]
static const float CHARGE_VBAT_EMERGENCY_OFF = 27.9F;  // [V]
static const float CHARGE_PWM_STEP           = 0.01F;  // [1]
static const float CHARGE_PWM_MIN            = 0.09F;  // [1]

Application::Application(System& system, Adc& adc, AnalogIn& vgen,
                         AnalogIn& vdc, AnalogIn& vbat, AnalogIn& ibat,
                         RpmMeasurement& rpmMeasurement,
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
    mRpmMeasurement(rpmMeasurement),
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
    mIncreasingDutyCycle(true),
    mPwmDutyCycle(0.0F),
    mPowerOffTimer(0U) {
}

void Application::runDisplayMode() {
  Enter();
  mRpmMeasurement.enable();
  mDisplay.switchOn();
  mDisplay.backlightOn();

  do {
    // update display for 10s
    for (int i = 0; i < 20; i++) {
      Measure();
      UpdateDisplay("U. Bruhin", true);
      mSystem.delay(500UL);
      Watchdog::reset();
    }
  } while (mButton1.read() == true);

  mDisplay.backlightOff();
  mDisplay.switchOff();
  mRpmMeasurement.disable();
  Exit();
}

void Application::runChargeMode() {
  Enter();

  // check voltages
  Measure();
  if ((mMeasuredVgen > START_VGEN_MIN) && (mMeasuredVdc < START_VDC_MAX) &&
      (mMeasuredVbat > START_VBAT_MIN) && (mMeasuredVbat < START_VBAT_MAX)) {
    // start charging
    mRpmMeasurement.enable();
    mDisplay.switchOn();
    mDisplay.backlightOn();
    mChargeEnable.setHigh();
    mPwm.enable();
    for (int i = 0; i < 20; i++) {
      Measure();
      SetPwmDutyCycle(START_PWM_DUTYCYCLE);
      UpdateDisplay("Starting");
      mSystem.delay(500UL);
      Watchdog::reset();
    }

    // charge
    do {
      Measure();
      if (mIncreasingDutyCycle) {
        mLedRed.setHigh();
        if (mMeasuredVbat < CHARGE_VBAT_MAX) {
          SetPwmDutyCycle(mMeasuredPotentiometer);
        } else {
          mIncreasingDutyCycle = false;
        }
      } else {
        mLedRed.toggle();
        if (mMeasuredVbat > CHARGE_VBAT_MAX) {
          SetPwmDutyCycle(mPwmDutyCycle - CHARGE_PWM_STEP);
        } else if (mMeasuredVbat < CHARGE_VBAT_MIN) {
          mIncreasingDutyCycle = true;
        }
      }
      UpdateDisplay(mPowerOffTimer ? "Stopping" : "Charge...");
      mSystem.delay(500UL);
      Watchdog::reset();

      if ((mMeasuredVdc < CHARGE_VDC_MIN) ||
          ((!mIncreasingDutyCycle) && (mPwmDutyCycle < CHARGE_PWM_MIN)) ||
          ((!mIncreasingDutyCycle) && (mMeasuredIbat < CHARGE_IBAT_MIN))) {
        mPowerOffTimer++;
      }
    } while ((mPowerOffTimer < 20U) &&
             (mMeasuredVbat < CHARGE_VBAT_EMERGENCY_OFF) &&
             (mButton2.read() == false));

    // stop charging
    mPwm.disable();
    UpdateDisplay("Finished!");
    System::delay(5000UL);  // avoid overvoltage due to energy in coil
    mChargeEnable.setLow();
    mDisplay.backlightOff();
    mDisplay.switchOff();
    mRpmMeasurement.disable();
  }

  Exit();
}

void Application::Enter() {
  mLedGreen.setHigh();
  mAdc.enable();
  mIncreasingDutyCycle = true;
  mPwmDutyCycle        = 0.0F;
  mPowerOffTimer       = 0U;
}

void Application::Exit() {
  mAdc.disable();
  mLedGreen.setLow();
  mLedRed.setLow();
  mSystem.sleep();
}

void Application::Measure() {
  mMeasuredIbat          = mIbat.measure();
  mMeasuredVgen          = mVgen.measure();
  mMeasuredVdc           = mVdc.measure();
  mMeasuredVbat          = mVbat.measure() - (0.1F * mMeasuredIbat);
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

void Application::UpdateDisplay(const char* msg, bool displayPotentiometer) {
  mDisplay.print(0, 0, "RPM%5d | GEN%5.1fV", mRpmMeasurement.getRpm(),
                 mMeasuredVgen);
  mDisplay.print(
      1, 0, "PWM%4.0f%% | VDC%5.1fV",
      (displayPotentiometer ? mMeasuredPotentiometer : mPwmDutyCycle) * 100.0F,
      mMeasuredVdc);
  mDisplay.print(2, 0, "CUR%4.1fA | BAT%5.1fV", mMeasuredIbat, mMeasuredVbat);
  mDisplay.print(3, 0, "PWR%4.0fW | %9s", mMeasuredVbat * mMeasuredIbat, msg);
}
