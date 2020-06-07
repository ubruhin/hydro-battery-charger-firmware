#pragma once

#include <stdint.h>

class Watchdog {
public:
  static void enable();
  static void reset();
};
