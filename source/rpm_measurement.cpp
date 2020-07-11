#include "rpm_measurement.h"

#include "input_edge_counter.h"

RpmMeasurement::RpmMeasurement(InputEdgeCounter& counter)
  : mCounter(counter), mTimeMs(0U), mRpm(0) {
}

void RpmMeasurement::enable() {
  mCounter.enable();
}

void RpmMeasurement::disable() {
  mCounter.disable();
}

void RpmMeasurement::sysTick() {
  ++mTimeMs;
  if (mTimeMs >= 1000UL) {
    mRpm    = mCounter.getValueAndReset() * 60UL / 14UL;
    mTimeMs = 0U;
  }
}

