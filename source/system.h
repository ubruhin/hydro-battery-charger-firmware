#include <stdint.h>

class System {
public:
  System();
  System(const System& other) = delete;

  bool GetWokeUpFromWatchdog() const { return mWokeUpFromWatchdog; }

  static void delay(uint32_t ms);
  static void sleep();

private:
  bool mWokeUpFromWatchdog;
};
