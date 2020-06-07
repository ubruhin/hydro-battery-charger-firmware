#include <stdint.h>

class System {
public:
  System();
  System(const System& other) = delete;

  bool getWokeUpFromWatchdog() const { return mWokeUpFromWatchdog; }

  static void delay(uint32_t ms);
  static void sleep();

private:
  bool mWokeUpFromWatchdog;
};
