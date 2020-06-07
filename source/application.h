#include <cstdint>

class Adc;
class AnalogIn;
class DigitalIn;
class DigitalOut;
class Display;
class Pwm;
class RpmMeasurement;
class System;

class Application {
public:
  Application()                         = delete;
  Application(const Application& other) = delete;
  Application(System& system, Adc& adc, AnalogIn& vgen, AnalogIn& vdc,
              AnalogIn& vbat, AnalogIn& ibat, RpmMeasurement& rpmMeasurement,
              DigitalOut& chargeEnable, Pwm& pwm, AnalogIn& pot,
              DigitalIn& button1, DigitalIn& button2, DigitalOut& ledGreen,
              DigitalOut& ledRed, Display& display);

  void runDisplayMode();
  void runChargeMode();

private:
  void Enter();
  void Exit();
  void Measure();
  void SetPwmDutyCycle(float value);
  void UpdateDisplay(const char* msg, bool displayPotentiometer = false);

  System&         mSystem;
  Adc&            mAdc;
  AnalogIn&       mVgen;
  AnalogIn&       mVdc;
  AnalogIn&       mVbat;
  AnalogIn&       mIbat;
  RpmMeasurement& mRpmMeasurement;
  DigitalOut&     mChargeEnable;
  Pwm&            mPwm;
  AnalogIn&       mPotentiometer;
  DigitalIn&      mButton1;
  DigitalIn&      mButton2;
  DigitalOut&     mLedGreen;
  DigitalOut&     mLedRed;
  Display&        mDisplay;

  float mMeasuredVgen;
  float mMeasuredVdc;
  float mMeasuredVbat;
  float mMeasuredIbat;
  float mMeasuredPotentiometer;

  bool     mIncreasingDutyCycle;
  float    mPwmDutyCycle;
  uint32_t mPowerOffTimer;
};
