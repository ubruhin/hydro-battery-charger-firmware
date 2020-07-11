#include "application.h"

#include "adc.h"
#include "analogin.h"
#include "digitalin.h"
#include "digitalout.h"
#include "display.h"
#include "pwm.h"
#include "rpm_measurement.h"
#include "system.h"
#include "valve.h"
#include "watchdog.h"

#include <cstdio>
#include <cstring>

// PWM parameters
static const float PWM_DUTY_CYCLE_STEPS = 0.002F;  // [1]
static const float PWM_DUTY_CYCLE_MIN   = 0.05F;   // [1]
static const float PWM_DUTY_CYCLE_MAX   = 0.99F;   // [1]

// start charging parameters
static const float START_VGEN_MIN      = 35.0F;  // [V]
static const float START_VDC_MAX       = 3.0F;   // [V]
static const float START_VBAT_MIN      = 22.0F;  // [V]
static const float START_VBAT_MAX      = 27.5F;  // [V]
static const float START_PWM_DUTYCYCLE = 0.2F;   // [1]
static const float AUTOSTART_VBAT_MAX  = 24.5F;  // [V]

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
                         RpmMeasurement& rpmMeasurement, Valve* valve,
                         DigitalOut& chargeEnable, Pwm& pwm, AnalogIn& pot,
                         DigitalIn& button1, DigitalIn& button2,
                         DigitalIn* autoStartSwitch, DigitalOut& powerLed,
                         DigitalOut& statusLed, Display& display)
  : mSystem(system),
    mAdc(adc),
    mVgen(vgen),
    mVdc(vdc),
    mVbat(vbat),
    mIbat(ibat),
    mRpmMeasurement(rpmMeasurement),
    mValve(valve),
    mChargeEnable(chargeEnable),
    mPwm(pwm),
    mPotentiometer(pot),
    mButton1(button1),
    mButton2(button2),
    mAutoStartSwitch(autoStartSwitch),
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
    mLastFulfilledChargeConditionsMs(0U),
    mMessage(""),
    mState(State::INIT),
    mStateTimeMs(0U),
    mLedBlinkRate(-1),
    mStartedByValve(false),
    mPoweredOn(false),
    mTotalTimeMs(0U),
    mLastProcessTimeMs(0U) {
}

void Application::run(bool displayMode) {
  // Setup
  mPowerLed.setHigh();
  mAdc.enable();
  mIncreasingDutyCycle = true;
  mPwmDutyCycle        = 0.0F;

  if (displayMode) {
    enterDisplayMode();
  } else {
    enterCheckStartConditions();
  }

  do {
    // Measure in 500ms interval to avoid flickering values
    if (mStateTimeMs % 500 == 0U) {
      measure();
    }

    // Process state machine
    processState();
    if (mState == State::FINISHED) {
      // Abort immediately (no delay, no display updace)
      break;
    }

    // Update display
    updateDisplay();

    // Blink status LED
    if (mLedBlinkRate < 0) {
      mStatusLed.setLow();
    } else if (mLedBlinkRate == 0) {
      mStatusLed.setHigh();
    } else if (mStateTimeMs % mLedBlinkRate == 0) {
      mStatusLed.toggle();
    }

    // Process valve
    if (mValve) {
      mValve->process(100U);
    }

    // Delay 100ms
    while (mTotalTimeMs < mLastProcessTimeMs + 100U)
      ;
    mLastProcessTimeMs = mTotalTimeMs;
    mStateTimeMs += 100U;

    Watchdog::reset();
  } while (true);

  // Power down
  mAdc.disable();
  mPowerLed.setLow();
  mStatusLed.setLow();
  mSystem.sleep();
}

void Application::sysTick() {
  ++mTotalTimeMs;
}

void Application::enterDisplayMode() {
  mRpmMeasurement.enable();
  mDisplay.switchOn();
  mDisplay.backlightOn();
  mPoweredOn    = true;
  mState        = State::DISPLAY_MODE;
  mStateTimeMs  = 0U;
  mLedBlinkRate = -1;
}

void Application::leaveDisplayMode(bool keepPoweredOn) {
  if (!keepPoweredOn) {
    mDisplay.backlightOff();
    mDisplay.switchOff();
    mRpmMeasurement.disable();
    mPoweredOn = false;
  }
  mState = State::FINISHED;
}

void Application::enterCheckStartConditions() {
  mState        = State::CHECK_START_CONDITIONS;
  mStateTimeMs  = 0U;
  mLedBlinkRate = -1;
}

void Application::leaveCheckStartConditions() {
  mState = State::FINISHED;
}

void Application::enterOpenValve() {
  if (!mPoweredOn) {
    mRpmMeasurement.enable();
    mDisplay.switchOn();
    mDisplay.backlightOn();
    mPoweredOn = true;
  }
  if (mValve) {
    mValve->open();
    mStartedByValve = true;
  }
  mState        = State::OPEN_VALVE;
  mStateTimeMs  = 0U;
  mLedBlinkRate = 200;
}

void Application::leaveOpenValve() {
  // Do not power-off valve, it might not be fully open yet!
  mState = State::FINISHED;
}

void Application::enterStartCharging(bool startedByValve) {
  if (!mPoweredOn) {
    mRpmMeasurement.enable();
    mDisplay.switchOn();
    mDisplay.backlightOn();
    mPoweredOn = true;
  }
  mChargeEnable.setHigh();
  mPwm.enable();
  mState          = State::START_CHARGING;
  mStateTimeMs    = 0U;
  mLedBlinkRate   = 600;
  mStartedByValve = startedByValve;
}

void Application::leaveStartCharging() {
  mState = State::FINISHED;
}

void Application::enterCharging() {
  mState                           = State::CHARGING;
  mStateTimeMs                     = 0U;
  mLedBlinkRate                    = 1000;
  mLastFulfilledChargeConditionsMs = 0U;
}

void Application::leaveCharging() {
  mState = State::FINISHED;
}

void Application::enterStopCharging() {
  if (mValve) {
    mValve->close();
  }
  mPwm.disable();
  mPwmDutyCycle = 0.0F;
  mState        = State::STOP_CHARGING;
  mStateTimeMs  = 0U;
  mLedBlinkRate = 100;
}

void Application::leaveStopCharging() {
  mChargeEnable.setLow();
  mDisplay.backlightOff();
  mDisplay.switchOff();
  mRpmMeasurement.disable();
  mPoweredOn = false;
  mState     = State::FINISHED;
}

void Application::processState() {
  switch (mState) {
    case State::DISPLAY_MODE: {
      checkOpenValveConditions();  // Updates mMessage
      if (mButton1.read()) {
        // Stay longer in this state
        mStateTimeMs = 0L;
      } else if (mButton2.read()) {
        // Wait until button released
        while (mButton2.read())
          ;
        // Force start charging
        leaveDisplayMode(true);
        enterOpenValve();
      } else if (mStateTimeMs >= 10000U) {
        // Go to sleep
        leaveDisplayMode(false);
      }
      break;
    }

    case State::CHECK_START_CONDITIONS: {
      if (checkStartConditions()) {  // Updates mMessage
        // Generator is already running, immediately start charging
        leaveCheckStartConditions();
        enterStartCharging(false);
      } else if (mAutoStartSwitch && mValve &&
                 checkOpenValveConditions()) {  // Updates mMessage
        // Battery low, auto-open valve to start charge
        leaveCheckStartConditions();
        enterOpenValve();
      } else {
        // Go to sleep
        leaveCheckStartConditions();
      }
      break;
    }

    case State::OPEN_VALVE: {
      if (checkStartConditions()) {  // Updates mMessage
        // Start charging
        leaveOpenValve();
        enterStartCharging(true);
      } else if (((mValve) && (mValve->isFullyOpen())) || (mButton2.read())) {
        // Close valve and power down
        leaveOpenValve();
        enterStopCharging();
      }
      break;
    }

    case State::START_CHARGING: {
      std::snprintf(mMessage, sizeof(mMessage), "Start...");
      setPwmDutyCycle(START_PWM_DUTYCYCLE);
      if (mStateTimeMs > 5000U) {
        leaveStartCharging();
        enterCharging();
      }
      break;
    }

    case State::CHARGING: {
      if (checkChargeConditions()) {  // Updates mMessage
        mLastFulfilledChargeConditionsMs = mStateTimeMs;
      }

      // Start closing valve if battery fully charged
      if (mStartedByValve && (mMeasuredVbat > CHARGE_VBAT_MAX)) {
        mValve->close();
        mLedBlinkRate = 100;
      }

      // Adjust PWM duty cycle
      if (mIncreasingDutyCycle) {
        if (mMeasuredVbat < CHARGE_VBAT_MAX) {
          setPwmDutyCycle(mMeasuredPotentiometer);
        } else {
          mIncreasingDutyCycle = false;
        }
      } else {
        if (mMeasuredVbat > CHARGE_VBAT_MAX) {
          setPwmDutyCycle(mPwmDutyCycle - CHARGE_PWM_STEP);
        } else if (mMeasuredVbat < CHARGE_VBAT_MIN) {
          mIncreasingDutyCycle = true;
        }
      }

      // Power off if finished charging
      if ((mStateTimeMs - mLastFulfilledChargeConditionsMs > 10000) ||
          (!(mMeasuredVbat < CHARGE_VBAT_EMERGENCY_OFF)) || (mButton2.read())) {
        leaveCharging();
        enterStopCharging();
      }
      break;
    }

    case State::STOP_CHARGING: {
      std::snprintf(mMessage, sizeof(mMessage), "Stopping");
      if ((mStateTimeMs > 5000U) &&
          ((!mStartedByValve) || (!mValve) || (mValve->isFullyClosed()))) {
        leaveStopCharging();
      }
      break;
    }

    default:
      mState = State::FINISHED;
      break;
  }
}

void Application::measure() {
  mMeasuredIbat          = mIbat.measure();
  mMeasuredVgen          = mVgen.measure();
  mMeasuredVdc           = mVdc.measure();
  mMeasuredVbat          = mVbat.measure() - (0.1F * mMeasuredIbat);
  mMeasuredPotentiometer = mPotentiometer.measure();
}

bool Application::checkOpenValveConditions() {
  if (mAutoStartSwitch && (!mAutoStartSwitch->read())) {
    std::snprintf(mMessage, sizeof(mMessage), "Power Off");
    return false;
  } else if (!(mMeasuredVbat >= START_VBAT_MIN)) {
    std::snprintf(mMessage, sizeof(mMessage), "BAT<%3.1fV", START_VBAT_MIN);
    return false;
  } else if (!(mMeasuredVbat <= AUTOSTART_VBAT_MAX)) {
    std::snprintf(mMessage, sizeof(mMessage), "BAT>%3.1fV", AUTOSTART_VBAT_MAX);
    return false;
  } else if (!(mMeasuredVdc <= START_VDC_MAX)) {
    std::snprintf(mMessage, sizeof(mMessage), "VDC>%3.1fV", START_VDC_MAX);
    return false;
  } else {
    std::snprintf(mMessage, sizeof(mMessage), "Ready");
    return true;
  }
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
    std::snprintf(mMessage, sizeof(mMessage), "CUR<%3.1fA", CHARGE_IBAT_MIN);
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

void Application::updateDisplay() {
  const float rpm = mRpmMeasurement.getRpm();
  const float pwm = mPwmDutyCycle;

  if (mDisplay.getColumnCount() >= 20) {
    mDisplay.print(0, 0, "RPM%5d | GEN%5.1fV", rpm, mMeasuredVgen);
    mDisplay.print(1, 0, "PWM%4.0f%% | VDC%5.1fV", pwm * 100.0F, mMeasuredVdc);
    mDisplay.print(2, 0, "CUR%4.1fA | BAT%5.1fV", mMeasuredIbat, mMeasuredVbat);
    mDisplay.print(3, 0, "PWR%4.0fW | %9s", mMeasuredVbat * mMeasuredIbat,
                   mMessage);
  } else {
    mDisplay.print(0, 0, "%4d%5.1fV", rpm, mMeasuredVgen);
    mDisplay.print(1, 0, "%3.0f%%%5.1fV", pwm * 100.0F, mMeasuredVdc);
    mDisplay.print(2, 0, "%3.1fA %4.1fV", mMeasuredIbat, mMeasuredVbat);
    mDisplay.print(3, 0, "%10s", mMessage);
  }
}
