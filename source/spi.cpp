#include "spi.h"

Spi::Spi(SPI_TypeDef* spi) : mSpi(spi) {
  LL_SPI_InitTypeDef params;
  LL_SPI_StructInit(&params);
  params.TransferDirection = LL_SPI_FULL_DUPLEX;
  params.Mode              = LL_SPI_MODE_MASTER;
  params.DataWidth         = LL_SPI_DATAWIDTH_8BIT;
  params.ClockPolarity     = LL_SPI_POLARITY_HIGH;
  params.ClockPhase        = LL_SPI_PHASE_2EDGE;
  params.NSS               = LL_SPI_NSS_SOFT;
  params.BaudRate          = LL_SPI_BAUDRATEPRESCALER_DIV16;
  params.BitOrder          = LL_SPI_LSB_FIRST;
  params.CRCCalculation    = LL_SPI_CRCCALCULATION_DISABLE;
  params.CRCPoly           = 7U;
  LL_SPI_Init(mSpi, &params);
}

void Spi::enable() {
  LL_SPI_Enable(mSpi);
}

void Spi::disable() {
  LL_SPI_Disable(mSpi);
}

void Spi::transceive(uint32_t len, const uint8_t* tx, uint8_t* rx) {
  for (uint32_t i = 0U; i < len; ++i) {
    LL_SPI_TransmitData8(mSpi, tx[i]);
    while (LL_SPI_IsActiveFlag_BSY(mSpi))
      ;
    if (rx) {
      rx[i] = LL_SPI_ReceiveData8(mSpi);
    }
  }
}
