#include "display.h"

#include "digitalout.h"
#include "spi.h"
#include "system.h"

#include <cstdio>
#include <cstring>

Display::Display(Spi& spi, DigitalOut& chipSelect, DigitalOut& enable,
                 DigitalOut& nrst)
  : mSpi(spi), mChipSelect(chipSelect), mEnable(enable), mNrst(nrst) {
}

void Display::switchOn() {
  mEnable.setLow();
  mSpi.enable();
  mNrst.setLow();
  System::delay(5);
  mNrst.setHigh();
  System::delay(100);

  writeCmd(0x3A);  // 8-Bit data length extension Bit RE=1; REV=0
  writeCmd(0x09);  // 4 line display
  writeCmd(0x06);  // Bottom view
  writeCmd(0x1E);  // Bias setting BS1=1
  writeCmd(0x39);  // 8-Bit data length extension Bit RE=0; IS=1
  writeCmd(0x1B);  // BS0=1 -> Bias=1/6
  writeCmd(0x6E);  // Devider on and set value
  writeCmd(0x56);  // Booster on and set contrast (BB1=C5, DB0=C4)
  writeCmd(0x7A);  // Set contrast (DB3-DB0=C3-C0)
  writeCmd(0x38);  // 8-Bit data length extension Bit RE=0; IS=0
  writeCmd(0x01);  // Clear
  writeCmd(0x0C);  // Display ON
}

void Display::switchOff() {
  mNrst.setLow();
  mSpi.disable();
  mEnable.setHigh();
}

void Display::clear() {
  writeCmd(0x01);
}

void Display::print(uint8_t row, uint8_t col, const char* format, ...) {
  va_list args;
  va_start(args, format);
  move(row, col);
  print(format, args);
  va_end(args);
}

void Display::move(uint8_t row, uint8_t col) {
  writeCmd(0x80 | (col + row * 0x20));
}

void Display::print(const char* format, va_list args) {
  char buf[21];
  int  len = std::vsnprintf(buf, sizeof(buf), format, args);
  if (len > 20) {
    len = 20;
  } else if (len < 0) {
    len = 0;
  }
  writeData(reinterpret_cast<const uint8_t*>(buf), len);
}

void Display::writeData(const uint8_t* data, uint32_t len) {
  write(STARTBYTE | FLAG_RS, data, len);
}

void Display::writeCmd(uint8_t cmd) {
  write(STARTBYTE, &cmd, 1U);
  while (readStatus() & FLAG_BUSY)
    ;
}

void Display::write(uint8_t header, const uint8_t* data, uint32_t len) {
  mChipSelect.setLow();
  mSpi.transceive(1U, &header, nullptr);
  for (uint32_t i = 0U; i < len; ++i) {
    uint8_t buffer[] = {static_cast<uint8_t>(data[i] & 0x0FU),
                        static_cast<uint8_t>(data[i] >> 4U)};
    mSpi.transceive(2U, buffer, nullptr);
  }
  mChipSelect.setHigh();
}

uint8_t Display::readStatus() {
  uint8_t tx[] = {STARTBYTE | FLAG_RW, 0x00};
  uint8_t rx[] = {0x00, 0x00};
  mChipSelect.setLow();
  mSpi.transceive(2U, tx, rx);
  mChipSelect.setHigh();
  return rx[0];
}
