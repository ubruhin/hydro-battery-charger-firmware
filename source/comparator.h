#include "stm32l0xx_ll_comp.h"

class Comparator {
public:
  typedef void (*Callback)();

  Comparator()                        = delete;
  Comparator(const Comparator& other) = delete;
  Comparator(COMP_TypeDef* comparator, uint32_t input, Callback callback);

  void enable();
  void disable();

private:
  COMP_TypeDef* mComparator;
};
