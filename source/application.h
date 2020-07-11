#pragma once

#include <cstdint>

class Adc;
class AnalogIn;
class DigitalIn;
class DigitalOut;
class Display;
class Pwm;
class RpmMeasurement;
class System;
class Valve;

class Application {
  enum class State {
    INIT,                    // Init state
    DISPLAY_MODE,            // Display mode
    CHECK_START_CONDITIONS,  // Decide whether and how to start charging
    OPEN_VALVE,      // Open valve and wait until start conditions are met
    START_CHARGING,  // Fixed PWM duty cycle for a fixed time
    CHARGING,        // Charging
    STOP_CHARGING,   // Stop charging and wait until valve is closed
    FINISHED,        // Everything finished, go to sleep
  };

public:
  Application()                         = delete;
  Application(const Application& other) = delete;
  Application(System& system, Adc& adc, AnalogIn& vgen, AnalogIn& vdc,
              AnalogIn& vbat, AnalogIn& ibat, RpmMeasurement& rpmMeasurement,
              Valve* valve, DigitalOut& chargeEnable, Pwm& pwm, AnalogIn& pot,
              DigitalIn& button1, DigitalIn& button2,
              DigitalIn* autoStartSwitch, DigitalOut& powerLed,
              DigitalOut& ledRed, Display& display);

  void run(bool displayMode);
  void sysTick();

private:
  void enterDisplayMode();
  void leaveDisplayMode(bool keepPoweredOn);
  void enterCheckStartConditions();
  void leaveCheckStartConditions();
  void enterOpenValve();
  void leaveOpenValve();
  void enterStartCharging(bool startedByValve);
  void leaveStartCharging();
  void enterCharging();
  void leaveCharging();
  void enterStopCharging();
  void leaveStopCharging();
  void processState();

  void measure();
  bool checkOpenValveConditions();
  bool checkStartConditions();
  bool checkChargeConditions();
  void setPwmDutyCycle(float value);
  void updateDisplay();

  System&         mSystem;
  Adc&            mAdc;
  AnalogIn&       mVgen;
  AnalogIn&       mVdc;
  AnalogIn&       mVbat;
  AnalogIn&       mIbat;
  RpmMeasurement& mRpmMeasurement;
  Valve*          mValve;
  DigitalOut&     mChargeEnable;
  Pwm&            mPwm;
  AnalogIn&       mPotentiometer;
  DigitalIn&      mButton1;
  DigitalIn&      mButton2;
  DigitalIn*      mAutoStartSwitch;
  DigitalOut&     mPowerLed;
  DigitalOut&     mStatusLed;
  Display&        mDisplay;

  float mMeasuredVgen;
  float mMeasuredVdc;
  float mMeasuredVbat;
  float mMeasuredIbat;
  float mMeasuredPotentiometer;

  bool     mIncreasingDutyCycle;
  float    mPwmDutyCycle;
  uint32_t mLastFulfilledChargeConditionsMs;
  char     mMessage[21];
  State    mState;
  uint32_t mStateTimeMs;
  int32_t  mLedBlinkRate;
  bool     mStartedByValve;
  bool     mPoweredOn;

  volatile uint32_t mTotalTimeMs;
  uint32_t          mLastProcessTimeMs;
};
