/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    spi.c
 * @brief   This file provides code for the configuration
 *          of the SPI instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include <string.h>

/* USER CODE BEGIN 0 */
#include "usart.h"
#include "ADXL355.h"
#define ADXL_RX_LEN 288
#define ADXL_CMD_REG FIFO_DATA // 替换为你的读寄存器起始地址
#define ADXL_SPI SPI1
#define ADXL_CS_PORT GPIOA
#define ADXL_CS_PIN GPIO_PIN_15

static uint8_t adxl_tx_buf[1 + ADXL_RX_LEN] __attribute__((aligned(4)));
static uint8_t adxl_rx_buf[1 + ADXL_RX_LEN] __attribute__((aligned(4)));
static volatile uint8_t adxl_busy = 0;

// Memory-to-Memory DMA 配置
#define ADXL_BUFFER_SIZE 10000  // 大缓冲区大小，可根据需要调整
static uint8_t adxl_large_buffer[ADXL_BUFFER_SIZE] __attribute__((aligned(4)));
static volatile uint16_t adxl_write_index = 0;  // 当前写入位置
static volatile uint8_t mem_dma_busy = 0;
DMA_HandleTypeDef handle_GPDMA1_Channel2;  // Memory-to-Memory DMA

static void adxl_cs_low(void) { HAL_GPIO_WritePin(ADXL_CS_PORT, ADXL_CS_PIN, GPIO_PIN_RESET); }
static void adxl_cs_high(void) { HAL_GPIO_WritePin(ADXL_CS_PORT, ADXL_CS_PIN, GPIO_PIN_SET); }
void ADXL355_DMA_Read_Prep(uint8_t start_reg)
{
  adxl_tx_buf[0] = (start_reg << 1) | 0x01; // 读命令
  for (uint16_t i = 1; i < sizeof(adxl_tx_buf); ++i)
    adxl_tx_buf[i] = 0x00; // dummy
}

void ADXL355_DMA_Start(void)
{
  //printf("ADXL355_DMA_Start1\r\n");
  if (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY || adxl_busy)
    return;
  //printf("ADXL355_DMA_Start2\r\n");
  adxl_busy = 1;
  adxl_cs_low();
  if (HAL_SPI_TransmitReceive_DMA(&hspi1, adxl_tx_buf, adxl_rx_buf, sizeof(adxl_tx_buf)) != HAL_OK)
  {
    //printf("ADXL355_DMA_Start3\r\n");
    adxl_cs_high();
    adxl_busy = 0;
  }
  //printf("ADXL355_DMA_Start4\r\n");
}

// 启动 Memory-to-Memory DMA 拷贝
static void Start_MemCopy_DMA(void)
{
  if (mem_dma_busy || adxl_write_index + ADXL_RX_LEN > ADXL_BUFFER_SIZE)
  {
    // DMA 忙或缓冲区满，跳过本次拷贝
    return;
  }
  
  mem_dma_busy = 1;
  
  // 启动 Memory-to-Memory DMA: 源=adxl_rx_buf[1], 目的=大缓冲区当前位置, 长度=288
  if (HAL_DMA_Start_IT(&handle_GPDMA1_Channel2, 
                       (uint32_t)&adxl_rx_buf[1], 
                       (uint32_t)&adxl_large_buffer[adxl_write_index], 
                       ADXL_RX_LEN) != HAL_OK)
  {
    mem_dma_busy = 0;
  }
}

// Memory-to-Memory DMA 完成回调
void HAL_DMA_MemCplt_Callback(DMA_HandleTypeDef *hdma)
{
  if (hdma->Instance == GPDMA1_Channel2)
  {
    adxl_write_index += ADXL_RX_LEN;
    mem_dma_busy = 0;
    printf("MemCopy Done, index=%d\r\n", adxl_write_index);
    
    // 可选：缓冲区满时的处理
    if (adxl_write_index >= ADXL_BUFFER_SIZE - ADXL_RX_LEN)
    {
      printf("Buffer Full!\r\n");
      // 这里可以触发数据处理或重置索引
      // adxl_write_index = 0;  // 循环缓冲
    }
  }
}

// 获取当前缓冲区数据量
uint16_t ADXL355_Get_Buffer_Count(void)
{
  return adxl_write_index;
}

// 读取并清空缓冲区
void ADXL355_Read_Buffer(uint8_t* dest, uint16_t* len)
{
  *len = adxl_write_index;
  if (*len > 0)
  {
    memcpy(dest, adxl_large_buffer, *len);
    adxl_write_index = 0;
  }
}

void get_adxl355_fifo_data(uint8_t *dat)
{
	
	//#warning 此函数消耗至少 ADXL355_LEN*9*2byte(若ADXL355_LEN=32,则消耗32*9*2=576byte) RAM 注意RAM是否够用
	uint8_t loop,loop_len,adxl355_max_xyz_len;
	
	//uint8_t dat[32*3*3]={0};//一组数据分为3个轴，一个轴3个byte数据，共ADXL355_LEN*3*3 byte
	struct ADXL355_Data_int32 buff[32] = {{0,0,0}};//缓存整理后的数据
	//先连续读取32组数据,然后再查找哪一组数据是X轴
	//SPI_ADXL355_Read_n_Bytes(FIFO_DATA,(uint8_t*)dat,ADXL355_LEN*3*3);
	
	//查找X轴 x轴的 AXIS MARKER为1 EMPTY INDICATOR为0
	loop_len = 32 * 3;//一组数据分为3个轴，共ADXL355_LEN*3个轴
	//printf("loop_len = %d \r\n",loop_len);
	
	
	//查找X轴 x轴的 AXIS MARKER为1 EMPTY INDICATOR为0
	loop_len = 32 * 3;	//一组数据分为3个轴，共ADXL355_LEN*3个轴的数据
	for(loop = loop_len;loop>0;loop--)
	{
		//printf("****X-AXIS MARKER %02x\r\n",(dat[loop*3]&0x03));
		if((dat[loop*3-1]&0x03) == 0x01) 
		{
			//printf("****X-AXIS MARKER %02x\r\n",dat[loop*3-1]);
			break;  	
		}
	}
	
	//printf("loop = %d \r\n",loop);
	//此时 loop的值为x轴数据的位置,一组数据有XYZ三轴
	adxl355_max_xyz_len = (loop+2)/3;//缓存中最大的X/Y/Z加速度的组数
	//printf("adxl355_max_xyz_len:%d\r\n",adxl355_max_xyz_len);
	
	//将原始数据解析成实际数据 只有21位有效值
	for(loop = 0;loop < adxl355_max_xyz_len;loop++)
	{
		buff[loop].i32SensorX=((uint32_t)dat[loop*9]<<16)|((uint32_t)dat[loop*9+1]<<8)|dat[loop*9+2];
		buff[loop].i32SensorX=ADXL355_Acceleration_Data_Conversion(buff[loop].i32SensorX);

		buff[loop].i32SensorY=((uint32_t)dat[loop*9+3]<<16)|((uint32_t)dat[loop*9+4]<<8)|dat[loop*9+5];
		buff[loop].i32SensorY=ADXL355_Acceleration_Data_Conversion(buff[loop].i32SensorY);

		buff[loop].i32SensorZ=((uint32_t)dat[loop*9+6]<<16)|((uint32_t)dat[loop*9+7]<<8)|dat[loop*9+8];
		buff[loop].i32SensorZ=ADXL355_Acceleration_Data_Conversion(buff[loop].i32SensorZ);
		
		//printf("0x%05lX  0x%05lX 0x%05lX\r\n",((int32_t)buff[loop].i32SensorX),((int32_t)buff[loop].i32SensorY),((int32_t)buff[loop].i32SensorZ));
		printf("%lf %lf %lf\r\n",((int32_t)buff[loop].i32SensorX)*15.6e-6, ((int32_t)buff[loop].i32SensorY)*15.6e-6,((int32_t)buff[loop].i32SensorZ)*15.6e-6);
	}
	
	// //获取数据，fifo,先进先出，最后的最新
	// if(adxl355_max_xyz_len==0)//没有有效的数据，返回buff[0],此时buff[0]的值全为0
	// {
	// 	printf("adxl355_max_xyz_len==0\r\n");
	// 	for(loop=0;loop<len;loop++)
	// 	{
	// 		target[loop] = buff[0];
	// 	}
	// }
	// else if(adxl355_max_xyz_len<len)//有效数据数量小于要获取的数据量
	// {
	// 	printf("adxl355_max_xyz_len<len\r\n");
	// 	for(loop=0;loop<adxl355_max_xyz_len;loop++)//先填充有效的数据
	// 	{
	// 		target[loop] = buff[loop];
	// 	}
		
	// 	for(loop=adxl355_max_xyz_len;loop<len;loop++)//不够的数据直接填充0
	// 	{
	// 		target[loop].i32SensorX = 0;
	// 		target[loop].i32SensorY = 0;
	// 		target[loop].i32SensorZ = 0;
	// 	}
	// }
	// else//有效数据数量大于要获取的数据量
	// {
	// 	//printf("other case\r\n");
	// 	for(loop=0;loop<len;loop++)
	// 	{
	// 		target[loop] = buff[loop];
	// 	}
	// }
}
/* USER CODE END 0 */

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef handle_GPDMA1_Channel0;
DMA_HandleTypeDef handle_GPDMA1_Channel1;
extern DMA_HandleTypeDef handle_GPDMA1_Channel2;

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */
  ADXL355_DMA_Read_Prep(FIFO_DATA);
  /* USER CODE END SPI1_Init 0 */

  SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x7;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  hspi1.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
  hspi1.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
  HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
  if (HAL_SPIEx_SetConfigAutonomousMode(&hspi1, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *spiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if (spiHandle->Instance == SPI1)
  {
    /* USER CODE BEGIN SPI1_MspInit 0 */

    /* USER CODE END SPI1_MspInit 0 */

    /** Initializes the peripherals clock
     */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
    PeriphClkInit.Spi1ClockSelection = RCC_SPI1CLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PB3 (JTDO/TRACESWO)     ------> SPI1_SCK
    PB4 (NJTRST)     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* GPDMA1_REQUEST_SPI1_RX Init */
    handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
    handle_GPDMA1_Channel0.Init.Request = GPDMA1_REQUEST_SPI1_RX;
    handle_GPDMA1_Channel0.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel0.Init.Direction = DMA_PERIPH_TO_MEMORY;
    handle_GPDMA1_Channel0.Init.SrcInc = DMA_SINC_FIXED;
    handle_GPDMA1_Channel0.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel0.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel0.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel0.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel0.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel0.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel0.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel0.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel0.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel0) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmarx, handle_GPDMA1_Channel0);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* GPDMA1_REQUEST_SPI1_TX Init */
    handle_GPDMA1_Channel1.Instance = GPDMA1_Channel1;
    handle_GPDMA1_Channel1.Init.Request = GPDMA1_REQUEST_SPI1_TX;
    handle_GPDMA1_Channel1.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel1.Init.Direction = DMA_MEMORY_TO_PERIPH;
    handle_GPDMA1_Channel1.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel1.Init.DestInc = DMA_DINC_FIXED;
    handle_GPDMA1_Channel1.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel1.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel1.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel1.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel1.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel1.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel1.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(spiHandle, hdmatx, handle_GPDMA1_Channel1);

    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel1, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }

    /* SPI1 interrupt Init */
    HAL_NVIC_SetPriority(SPI1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

    /* USER CODE BEGIN SPI1_MspInit 1 */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
    /* Optional: Enable TX DMA channel IRQ if not enabled elsewhere */
    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
    
    /* Memory-to-Memory DMA Init (Channel 2) */
    handle_GPDMA1_Channel2.Instance = GPDMA1_Channel2;
    handle_GPDMA1_Channel2.Init.Request = DMA_REQUEST_SW;  // 软件触发
    handle_GPDMA1_Channel2.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    handle_GPDMA1_Channel2.Init.Direction = DMA_MEMORY_TO_MEMORY;
    handle_GPDMA1_Channel2.Init.SrcInc = DMA_SINC_INCREMENTED;
    handle_GPDMA1_Channel2.Init.DestInc = DMA_DINC_INCREMENTED;
    handle_GPDMA1_Channel2.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel2.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    handle_GPDMA1_Channel2.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    handle_GPDMA1_Channel2.Init.SrcBurstLength = 1;
    handle_GPDMA1_Channel2.Init.DestBurstLength = 1;
    handle_GPDMA1_Channel2.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    handle_GPDMA1_Channel2.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    handle_GPDMA1_Channel2.Init.Mode = DMA_NORMAL;
    if (HAL_DMA_Init(&handle_GPDMA1_Channel2) != HAL_OK)
    {
      Error_Handler();
    }
    
    if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel2, DMA_CHANNEL_NPRIV) != HAL_OK)
    {
      Error_Handler();
    }
    
    // 注册 Memory-to-Memory DMA 完成回调
    HAL_DMA_RegisterCallback(&handle_GPDMA1_Channel2, HAL_DMA_XFER_CPLT_CB_ID, HAL_DMA_MemCplt_Callback);
    
    HAL_NVIC_SetPriority(GPDMA1_Channel2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel2_IRQn);
    
    /* USER CODE END SPI1_MspInit 1 */
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *spiHandle)
{

  if (spiHandle->Instance == SPI1)
  {
    /* USER CODE BEGIN SPI1_MspDeInit 0 */

    /* USER CODE END SPI1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    /**SPI1 GPIO Configuration
    PB3 (JTDO/TRACESWO)     ------> SPI1_SCK
    PB4 (NJTRST)     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);

    /* SPI1 DMA DeInit */
    HAL_DMA_DeInit(spiHandle->hdmarx);

    /* SPI1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(SPI1_IRQn);
    /* USER CODE BEGIN SPI1_MspDeInit 1 */

    /* USER CODE END SPI1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

// 修改后的回调函数
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == ADXL_SPI)
  {
    adxl_cs_high();
    // get_adxl355_fifo_data(&adxl_rx_buf[1]);
    
    // SPI DMA 完成后，启动 Memory-to-Memory DMA 拷贝到大缓冲区
    Start_MemCopy_DMA();
    
    adxl_busy = 0;
    //printf("SPI DMA Complete\r\n");
  }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_7) // PB7
  {
    printf("INT\r\n");
     ADXL355_DMA_Start();
    //SPI_ADXL355_Read_n_Bytes(FIFO_DATA,(uint8_t*)adxl_rx_buf,32*3*3);

  }
}

// SPI 错误回调
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == ADXL_SPI)
  {
    adxl_cs_high();
    adxl_busy = 0;
  }
}

/* USER CODE END 1 */
