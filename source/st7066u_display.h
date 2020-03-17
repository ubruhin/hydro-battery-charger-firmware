#include "display.h"

#include <cstdarg>
#include <stdint.h>

class DigitalOut;

class ST7066UDisplay : public Display {
public:
  ST7066UDisplay()                            = delete;
  ST7066UDisplay(const ST7066UDisplay& other) = delete;
  ST7066UDisplay(DigitalOut& power, DigitalOut& rs, DigitalOut& rw,
                 DigitalOut& enable, DigitalOut& db4, DigitalOut& db5,
                 DigitalOut& db6, DigitalOut& db7, DigitalOut& backlight);

  virtual void switchOn();
  virtual void switchOff();
  virtual void backlightOn();
  virtual void backlightOff();
  virtual void clear();
  virtual void print(uint8_t row, uint8_t col, const char* format, ...);

private:
  void    move(uint8_t row, uint8_t col);
  void    print(const char* format, va_list args);
  void    writeData(const uint8_t* data, uint32_t len);
  void    writeCmd(uint8_t cmd);
  void    write(const uint8_t* data, uint32_t len);
  void    write4bit(uint8_t data);
  void    write1bit(DigitalOut& io, uint8_t data);
  void    sendEnablePulse();
  uint8_t readStatus();

  DigitalOut& mPower;
  DigitalOut& mRS;
  DigitalOut& mRW;
  DigitalOut& mEnable;
  DigitalOut& mDB4;
  DigitalOut& mDB5;
  DigitalOut& mDB6;
  DigitalOut& mDB7;
  DigitalOut& mBacklight;
  bool        mIsOn;

  static const uint8_t FLAG_BUSY = 0b10000000;
};
