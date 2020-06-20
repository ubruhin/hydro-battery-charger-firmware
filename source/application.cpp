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

#include <cstdio>
#include <cstring>

// PWM parameters
static const float PWM_DUTY_CYCLE_STEPS = 0.01F;  // [1]
static const float PWM_DUTY_CYCLE_MIN   = 0.05F;  // [1]
static const float PWM_DUTY_CYCLE_MAX   = 0.99F;  // [1]

// start charging parameters
static const float START_VGEN_MIN      = 30.0F;  // [V]
static const float START_VDC_MAX       = 3.0F;   // [V]
static const float START_VBAT_MIN      = 22.0F;  // [V]
static const float START_VBAT_MAX      = 27.5F;  // [V]
static const float START_PWM_DUTYCYCLE = 0.2F;   // [1]

// charging parameters
static const float CHARGE_VDC_MIN            = 20.0F;  // [V]
static const float CHARGE_IBAT_MIN           = 0.1F;   // [A]
static const float CHARGE_VBAT_MIN           = 27.8F;  // [V]
static const float CHARGE_VBAT_MAX           = 28.0F;  // [V]
static const float CHARGE_VBAT_EMERGENCY_OFF = 28.5F;  // [V]
static const float CHARGE_PWM_STEP           = 0.01F;  // [1]
static const float CHARGE_PWM_MIN            = 0.09F;  // [1]

Application::Application(System& system, Adc& adc, AnalogIn& vgen,
                         AnalogIn& vdc, AnalogIn& vbat, AnalogIn& ibat,
                         RpmMeasurement& rpmMeasurement,
                         DigitalOut& chargeEnable, Pwm& pwm, AnalogIn& pot,
                         DigitalIn& button1, DigitalIn& button2,
                         DigitalOut& powerLed, DigitalOut& statusLed,
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
    mPowerLed(powerLed),
    mStatusLed(statusLed),
    mDisplay(display),
    mMeasuredVgen(0.0F),
    mMeasuredVdc(0.0F),
    mMeasuredVbat(0.0F),
    mMeasuredIbat(0.0F),
    mMeasuredPotentiometer(0.0F),
    mIncreasingDutyCycle(true),
    mPwmDutyCycle(0.0F),
    mPowerOffTimer(0U),
    mMessage("") {
}

void Application::run(bool displayMode) {
  enter();

  bool start = false;

  // display mode
  if (displayMode) {
    mRpmMeasurement.enable();
    mDisplay.switchOn();
    mDisplay.backlightOn();

    do {
      // update display for 10s
      for (int i = 0; i < 20; i++) {
        measure();
        checkStartConditions();  // Updates mMessage
        updateDisplay(mMessage, true);
        mSystem.delay(500UL);
        Watchdog::reset();
        if (mButton2.read()) {
          start = true;
          break;
        }
      }
    } while (mButton1.read() == true);

    mDisplay.backlightOff();
    mDisplay.switchOff();
    mRpmMeasurement.disable();
  }

  // check start conditions
  if (!start) {
    measure();
    start = checkStartConditions();
  }

  // charge mode
  if (start) {
    // start charging
    mRpmMeasurement.enable();
    mDisplay.switchOn();
    mDisplay.backlightOn();
    mChargeEnable.setHigh();
    mPwm.enable();
    for (int i = 0; i < 20; i++) {
      measure();
      setPwmDutyCycle(START_PWM_DUTYCYCLE);
      updateDisplay("Starting");
      mSystem.delay(500UL);
      Watchdog::reset();
    }

    // charge
    do {
      measure();
      if (checkChargeConditions()) {
        mPowerOffTimer = 0U;
      } else {
        mPowerOffTimer++;
      }
      if (mIncreasingDutyCycle) {
        mStatusLed.setHigh();
        if (mMeasuredVbat < CHARGE_VBAT_MAX) {
          setPwmDutyCycle(mMeasuredPotentiometer);
        } else {
          mIncreasingDutyCycle = false;
        }
      } else {
        mStatusLed.toggle();
        if (mMeasuredVbat > CHARGE_VBAT_MAX) {
          setPwmDutyCycle(mPwmDutyCycle - CHARGE_PWM_STEP);
        } else if (mMeasuredVbat < CHARGE_VBAT_MIN) {
          mIncreasingDutyCycle = true;
        }
      }
      updateDisplay(mMessage);
      mSystem.delay(500UL);
      Watchdog::reset();
    } while ((mPowerOffTimer < 20U) &&
             (mMeasuredVbat < CHARGE_VBAT_EMERGENCY_OFF) &&
             (mButton2.read() == false));

    // stop charging
    mPwm.disable();
    updateDisplay("Finished!");
    System::delay(5000UL);  // avoid overvoltage due to energy in coil
    mChargeEnable.setLow();
    mDisplay.backlightOff();
    mDisplay.switchOff();
    mRpmMeasurement.disable();
  }

  exit();
}

void Application::enter() {
  mPowerLed.setHigh();
  mAdc.enable();
  mIncreasingDutyCycle = true;
  mPwmDutyCycle        = 0.0F;
  mPowerOffTimer       = 0U;
}

void Application::exit() {
  mAdc.disable();
  mPowerLed.setLow();
  mStatusLed.setLow();
  mSystem.sleep();
}

void Application::measure() {
  mMeasuredIbat          = mIbat.measure();
  mMeasuredVgen          = mVgen.measure();
  mMeasuredVdc           = mVdc.measure();
  mMeasuredVbat          = mVbat.measure() - (0.1F * mMeasuredIbat);
  mMeasuredPotentiometer = mPotentiometer.measure();
}

bool Application::checkStartConditions() {
  if (!(mMeasuredVgen >= START_VGEN_MIN)) {
    std::snprintf(mMessage, sizeof(mMessage), "GEN<%3.1fV", START_VGEN_MIN);
    return false;
  } else if (!(mMeasuredVbat >= START_VBAT_MIN)) {
    std::snprintf(mMessage, sizeof(mMessage), "BAT<%3.1fV", START_VBAT_MIN);
    return false;
  } else if (!(mMeasuredVbat <= START_VBAT_MAX)) {
    std::snprintf(mMessage, sizeof(mMessage), "BAT>%3.1fV", START_VBAT_MAX);
    return false;
  } else if (!(mMeasuredVdc <= START_VDC_MAX)) {
    std::snprintf(mMessage, sizeof(mMessage), "VDC>%3.1fV", START_VDC_MAX);
    return false;
  } else {
    std::snprintf(mMessage, sizeof(mMessage), "Ready");
    return true;
  }
}

bool Application::checkChargeConditions() {
  if (!(mMeasuredVdc >= CHARGE_VDC_MIN)) {
    std::snprintf(mMessage, sizeof(mMessage), "VDC<%3.1fV", CHARGE_VDC_MIN);
    return false;
  } else if ((!(mPwmDutyCycle >= CHARGE_PWM_MIN)) && (!mIncreasingDutyCycle)) {
    std::snprintf(mMessage, sizeof(mMessage), "PWM<%3.0fV", CHARGE_PWM_MIN);
    return false;
  } else if ((!(mMeasuredIbat >= CHARGE_IBAT_MIN)) && (!mIncreasingDutyCycle)) {
    std::snprintf(mMessage, sizeof(mMessage), "CUR<%3.1fV", CHARGE_IBAT_MIN);
    return false;
  } else {
    std::snprintf(mMessage, sizeof(mMessage), "Charge...");
    return true;
  }
}

void Application::setPwmDutyCycle(float value) {
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

void Application::updateDisplay(const char* msg, bool displayMode) {
  const float rpm = mRpmMeasurement.getRpm();
  const float pwm = displayMode ? mMeasuredPotentiometer : mPwmDutyCycle;

  if (mDisplay.getColumnCount() >= 20) {
    mDisplay.print(0, 0, "RPM%5d | GEN%5.1fV", rpm, mMeasuredVgen);
    mDisplay.print(1, 0, "PWM%4.0f%% | VDC%5.1fV", pwm * 100.0F, mMeasuredVdc);
    mDisplay.print(2, 0, "CUR%4.1fA | BAT%5.1fV", mMeasuredIbat, mMeasuredVbat);
    mDisplay.print(3, 0, "PWR%4.0fW | %9s", mMeasuredVbat * mMeasuredIbat, msg);
  } else {
    mDisplay.print(0, 0, "%4d%5.1fV", rpm, mMeasuredVgen);
    mDisplay.print(1, 0, "%3.0f%%%5.1fV", pwm * 100.0F, mMeasuredVdc);
    mDisplay.print(2, 0, "%3.1fA %4.1fV", mMeasuredIbat, mMeasuredVbat);
    mDisplay.print(3, 0, "%10s", msg);
  }
}
