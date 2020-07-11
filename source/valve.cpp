#include "valve.h"

Valve::Valve(DigitalOut& power, DigitalOut& open)
  : mPower(power), mOpen(open), mState(State::UNKNOWN), mTimeMs(0U) {
}

void Valve::open() {
  if ((mState != State::OPENING) && (mState != State::FULLY_OPEN)) {
    mPower.setHigh();
    mOpen.setHigh();
    mState  = State::OPENING;
    mTimeMs = 0U;
  }
}

void Valve::close() {
  if ((mState != State::CLOSING) && (mState != State::FULLY_CLOSED)) {
    mPower.setHigh();
    mOpen.setLow();
    mState  = State::CLOSING;
    mTimeMs = 0U;
  }
}

void Valve::process(uint32_t intervalMs) {
  mTimeMs += intervalMs;

  if (mTimeMs > 150000U) {
    if (mState == State::OPENING) {
      mState = State::FULLY_OPEN;
    } else if (mState == State::CLOSING) {
      mState = State::FULLY_CLOSED;
    }
    mOpen.setLow();
    mPower.setLow();
  }
}
