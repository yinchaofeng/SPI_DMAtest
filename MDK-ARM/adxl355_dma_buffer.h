/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    adxl355_dma_buffer.h
 * @brief   ADXL355 DMA Buffer Management Header
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef __ADXL355_DMA_BUFFER_H
#define __ADXL355_DMA_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 函数声明 */
void ADXL355_DMA_Read_Prep(uint8_t start_reg);
void ADXL355_DMA_Start(void);
uint16_t ADXL355_Get_Buffer_Count(void);
void ADXL355_Read_Buffer(uint8_t* dest, uint16_t* len);

#ifdef __cplusplus
}
#endif

#endif /* __ADXL355_DMA_BUFFER_H */
