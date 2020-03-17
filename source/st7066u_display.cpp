#include "st7066u_display.h"

#include "digitalout.h"
#include "system.h"

#include <cstdio>
#include <cstring>

ST7066UDisplay::ST7066UDisplay(DigitalOut& power, DigitalOut& rs,
                               DigitalOut& rw, DigitalOut& enable,
                               DigitalOut& db4, DigitalOut& db5,
                               DigitalOut& db6, DigitalOut& db7,
                               DigitalOut& backlight)
  : Display(),
    mPower(power),
    mRS(rs),
    mRW(rw),
    mEnable(enable),
    mDB4(db4),
    mDB5(db5),
    mDB6(db6),
    mDB7(db7),
    mBacklight(backlight),
    mIsOn(false) {
}

void ST7066UDisplay::switchOn() {
  // power-on
  mPower.setLow();
  mBacklight.setHigh();
  System::delay(100);
  mIsOn = true;

  // enter 4-bit mode
  write4bit(0x03);
  System::delay(30);
  sendEnablePulse();
  System::delay(10);
  sendEnablePulse();
  System::delay(10);
  sendEnablePulse();
  System::delay(10);
  write4bit(0x02);
  sendEnablePulse();

  // initialize
  writeCmd(0x2A);  // 4-Bit data length extension Bit RE=1; REV=0
  writeCmd(0x09);  // 4 line display
  writeCmd(0x06);  // Bottom view
  writeCmd(0x1E);  // Bias setting BS1=1
  writeCmd(0x29);  // 4-Bit data length extension Bit RE=0; IS=1
  writeCmd(0x1B);  // BS0=1 -> Bias=1/6
  writeCmd(0x6E);  // Devider on and set value
  writeCmd(0x56);  // Booster on and set contrast (BB1=C5, DB0=C4)
  writeCmd(0x7A);  // Set contrast (DB3-DB0=C3-C0)
  writeCmd(0x28);  // 4-Bit data length extension Bit RE=0; IS=0
  writeCmd(0x01);  // Clear
  writeCmd(0x0C);  // Display ON
}

void ST7066UDisplay::switchOff() {
  mDB4.setLow();
  mDB5.setLow();
  mDB6.setLow();
  mDB7.setLow();
  mRW.setLow();
  mRS.setLow();
  mEnable.setLow();
  mBacklight.setLow();
  mPower.setHigh();
  mIsOn = false;
}

void ST7066UDisplay::backlightOn() {
  mBacklight.setLow();
}

void ST7066UDisplay::backlightOff() {
  if (mIsOn) {
    mBacklight.setHigh();
  }
}

void ST7066UDisplay::clear() {
  writeCmd(0x01);
}

void ST7066UDisplay::print(uint8_t row, uint8_t col, const char* format, ...) {
  va_list args;
  va_start(args, format);
  move(row, col);
  print(format, args);
  va_end(args);
}

void ST7066UDisplay::move(uint8_t row, uint8_t col) {
  uint8_t rowOffsets[] = {0x00U, 0x40U, 0x00U + 20U, 0x40U + 20U};
  writeCmd(0x80 | (col + rowOffsets[row]));
}

void ST7066UDisplay::print(const char* format, va_list args) {
  char buf[21];
  int  len = std::vsnprintf(buf, sizeof(buf), format, args);
  if (len > 20) {
    len = 20;
  } else if (len < 0) {
    len = 0;
  }
  writeData(reinterpret_cast<const uint8_t*>(buf), len);
}

void ST7066UDisplay::writeData(const uint8_t* data, uint32_t len) {
  mRS.setHigh();
  write(data, len);
}

void ST7066UDisplay::writeCmd(uint8_t cmd) {
  mRS.setLow();
  write(&cmd, 1U);
  while (readStatus() & FLAG_BUSY)
    ;
}

void ST7066UDisplay::write(const uint8_t* data, uint32_t len) {
  mRW.setLow();
  for (uint32_t i = 0U; i < len; ++i) {
    write4bit(static_cast<uint8_t>(data[i] >> 4U));
    sendEnablePulse();
    write4bit(static_cast<uint8_t>(data[i] & 0x0FU));
    sendEnablePulse();
  }
}

void ST7066UDisplay::write4bit(uint8_t data) {
  write1bit(mDB4, data & 0x01U);
  write1bit(mDB5, data & 0x02U);
  write1bit(mDB6, data & 0x04U);
  write1bit(mDB7, data & 0x08U);
}

void ST7066UDisplay::write1bit(DigitalOut& io, uint8_t data) {
  if (data) {
    io.setHigh();
  } else {
    io.setLow();
  }
}

void ST7066UDisplay::sendEnablePulse() {
  mEnable.setHigh();
  System::delay(1);
  mEnable.setLow();
  System::delay(1);
}

uint8_t ST7066UDisplay::readStatus() {
  mRW.setHigh();
  return 0x00U;  // Not supported yet
}
