#pragma once

#include "digitalout.h"

class Valve {
  enum class State {
    UNKNOWN,
    OPENING,
    CLOSING,
    FULLY_OPEN,
    FULLY_CLOSED,
  };

public:
  Valve()                   = delete;
  Valve(const Valve& other) = delete;
  Valve(DigitalOut& power, DigitalOut& open);

  void open();
  void close();
  bool isFullyOpen() const { return mState == State::FULLY_OPEN; }
  bool isFullyClosed() const { return mState == State::FULLY_CLOSED; }
  void process(uint32_t intervalMs);

private:
  DigitalOut& mPower;
  DigitalOut& mOpen;
  State       mState;
  uint32_t    mTimeMs;
};
