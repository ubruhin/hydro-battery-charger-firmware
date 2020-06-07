#pragma once

#include <cstdarg>
#include <stdint.h>

class Display {
public:
  Display() {}
  Display(const Display& other) = delete;
  virtual ~Display() {}

  virtual int  getColumnCount() const                                   = 0;
  virtual void switchOn()                                               = 0;
  virtual void switchOff()                                              = 0;
  virtual void backlightOn()                                            = 0;
  virtual void backlightOff()                                           = 0;
  virtual void clear()                                                  = 0;
  virtual void print(uint8_t row, uint8_t col, const char* format, ...) = 0;
};
