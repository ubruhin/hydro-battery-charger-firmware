#include "adc.h"

#include "system.h"

Adc::Adc(ADC_TypeDef* adc) : mAdc(adc) {
}

void Adc::enable() {
  LL_ADC_InitTypeDef params;
  LL_ADC_StructInit(&params);
  params.Clock         = LL_ADC_CLOCK_SYNC_PCLK_DIV2;
  params.Resolution    = LL_ADC_RESOLUTION_12B;
  params.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  params.LowPowerMode  = LL_ADC_LP_MODE_NONE;
  LL_ADC_Init(mAdc, &params);

  LL_ADC_EnableInternalRegulator(mAdc);
  System::delay(50UL);

  LL_ADC_StartCalibration(mAdc);
  while (LL_ADC_IsCalibrationOnGoing(mAdc))
    ;
  System::delay(50UL);

  LL_ADC_Enable(mAdc);
  while (LL_ADC_IsActiveFlag_ADRDY(mAdc) == 0)
    ;

  LL_ADC_REG_InitTypeDef regularParams;
  LL_ADC_REG_StructInit(&regularParams);
  regularParams.TriggerSource    = LL_ADC_REG_TRIG_SOFTWARE;
  regularParams.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
  regularParams.ContinuousMode   = LL_ADC_REG_CONV_SINGLE;
  regularParams.DMATransfer      = LL_ADC_REG_DMA_TRANSFER_NONE;
  regularParams.Overrun          = LL_ADC_REG_OVR_DATA_PRESERVED;
  LL_ADC_REG_Init(mAdc, &regularParams);
}

void Adc::disable() {
  LL_ADC_Disable(mAdc);
  LL_ADC_DisableInternalRegulator(mAdc);
}

float Adc::measureVolts(uint8_t channel) {
  LL_ADC_REG_SetSequencerChannels(mAdc, channel);
  LL_ADC_REG_StartConversion(mAdc);
  while (LL_ADC_REG_IsConversionOngoing(mAdc))
    ;
  uint32_t ticks = LL_ADC_REG_ReadConversionData32(mAdc);
  return static_cast<float>(ticks) * 3.3F / 4095.0F;
}
