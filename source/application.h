class Adc;
class AnalogIn;
class DigitalIn;
class DigitalOut;
class Display;
class Pwm;
class System;

class Application {
public:
  Application()                         = delete;
  Application(const Application& other) = delete;
  Application(System& system, Adc& adc, AnalogIn& vgen, AnalogIn& vdc,
              AnalogIn& vbat, AnalogIn& ibat, DigitalOut& chargeEnable,
              Pwm& pwm, AnalogIn& pot, DigitalIn& button1, DigitalIn& button2,
              DigitalOut& ledGreen, DigitalOut& ledRed, Display& display);

  void process();

private:
  void Measure();
  void SetPwmDutyCycle(float value);
  void UpdateDisplay(const char* msg);

  System&     mSystem;
  Adc&        mAdc;
  AnalogIn&   mVgen;
  AnalogIn&   mVdc;
  AnalogIn&   mVbat;
  AnalogIn&   mIbat;
  DigitalOut& mChargeEnable;
  Pwm&        mPwm;
  AnalogIn&   mPotentiometer;
  DigitalIn&  mButton1;
  DigitalIn&  mButton2;
  DigitalOut& mLedGreen;
  DigitalOut& mLedRed;
  Display&    mDisplay;

  float mMeasuredVgen;
  float mMeasuredVdc;
  float mMeasuredVbat;
  float mMeasuredIbat;
  float mMeasuredPotentiometer;

  float mPwmDutyCycle;
};
