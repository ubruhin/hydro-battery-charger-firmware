#include <stdint.h>

class System {
public:
  System();
  System(const System& other) = delete;

  static void delay(uint32_t ms);
  static void sleep();
};
