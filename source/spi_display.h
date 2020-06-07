#pragma once

#include "display.h"

#include <cstdarg>
#include <stdint.h>

class Spi;
class DigitalOut;

class SpiDisplay : public Display {
public:
  SpiDisplay()                        = delete;
  SpiDisplay(const SpiDisplay& other) = delete;
  SpiDisplay(Spi& spi, DigitalOut& chipSelect, DigitalOut& enable,
             DigitalOut& nrst);

  virtual int  getColumnCount() const { return 10; }
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
  void    write(uint8_t header, const uint8_t* data, uint32_t len);
  uint8_t readStatus();

  Spi&        mSpi;
  DigitalOut& mChipSelect;
  DigitalOut& mEnable;
  DigitalOut& mNrst;

  static const uint8_t STARTBYTE = 0b00011111;
  static const uint8_t FLAG_RW   = 0b00100000;
  static const uint8_t FLAG_RS   = 0b01000000;
  static const uint8_t FLAG_BUSY = 0b10000000;
};
