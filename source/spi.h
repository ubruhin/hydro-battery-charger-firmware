#include "stm32l0xx_ll_spi.h"

class Spi {
public:
  Spi()                 = delete;
  Spi(const Spi& other) = delete;
  Spi(SPI_TypeDef* spi);

  void enable();
  void disable();
  void transceive(uint32_t len, const uint8_t* tx, uint8_t* rx);

private:
  SPI_TypeDef* mSpi;
};
