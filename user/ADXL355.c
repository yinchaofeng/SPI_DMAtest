#include "ADXL355.h"
#include "main.h"


#define ADXL357 0

#define ADXL355_LEN 32  //最多读取32组数据 X/Y/Z加速度数据 每个X/Y/Z数组3个字节 共有 32x3x3 = 288字节

//struct ADXL355_Data acc;

uint8_t range = 8;
uint8_t samples = 0x02;
//float32_t threshold_adxl355 = 1.2;




//int32_t volatile i32SensorT; //温度

//原始的无符号加速度数据
uint32_t volatile ui32SensorX;
uint32_t volatile ui32SensorY;
uint32_t volatile ui32SensorZ;  

//计算标志，用于区分需要计算的数据 
uint8_t cal_flag = 0; 

//355上电
void ADXL355_Power_On(void)
{
	//HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1,GPIO_PIN_SET); //打开指示灯
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,GPIO_PIN_SET); //ADXL355上电
	HAL_Delay(20);
}


void ADXL355_Power_Off(void)
{
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,GPIO_PIN_RESET); //ADXL355断电
	//HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1,GPIO_PIN_RESET); //关闭指示灯
}


void ADXL355_Init(void) 
{
   //SPI_ADXL355_Write_Byte(ADX_RESET, 0x52);
   uint8_t volatile  devid_ad = SPI_ADXL355_Read_Byte(DEVID_AD);                  /* Read the ID register */
   uint8_t volatile devid_mst = SPI_ADXL355_Read_Byte(DEVID_MST);                  /* Read the ID register */
   uint8_t volatile partid = SPI_ADXL355_Read_Byte(PARTID);                  /* Read the ID register */
   uint8_t volatile revid = SPI_ADXL355_Read_Byte(REVID);                 /* Read the ID register */
	
	devid_ad = SPI_ADXL355_Read_Byte(DEVID_AD); //有一定概率出现 DEVID_AD = 0xFF的情况出现，再读一次
	devid_mst = SPI_ADXL355_Read_Byte(DEVID_MST);
	partid = SPI_ADXL355_Read_Byte(PARTID);  
	revid = SPI_ADXL355_Read_Byte(REVID); 
	
	if((devid_ad == 0xAD) && (devid_mst == 0x1D) && (partid == 0xED) && (revid == 0x01)) 
	{
	   printf("\n\rReset and initialized.\n\r");
	   SPI_ADXL355_Write_Byte(ADX_RESET, 0x52);   //向Reset寄存器写入0x52  重置ADXL355
	}
	else
	{
		printf("devid_ad = 0x%02x ---- 0xAD\r\n",devid_ad);
		printf("devid_mst = 0x%02x --- 0x1D\r\n",devid_mst);
		printf("partid = 0x%02x --- 0xED\r\n",partid);
		printf("revid = 0x%02x --- 0x01\r\n",revid);	
		printf("Error initializing\n\r");
	}
}		
	   


//振动采集
void ADXL355_Vibration_Config(float32_t *fsr)
{
	ADXL355_Stop_Sensor();   //停止采集加速度
	//HAL_NVIC_DisableIRQ(EXTI7_IRQn); //关闭EXIT7中断
	SPI_ADXL355_Write_Byte(ADX_RESET, 0x52);   //向Reset 寄存器写入0x52  重置ADXL355
	HAL_Delay(2);  //新增一定的延时时间
	uint8_t receive = 0x00;
	//设置满量程 8g
	ADXL355_Set_Range(range); //参数2、4、8	
	receive = ADXL355_Read_Range();
	range = receive;
	
#if ADXL357  //判断是否为ADXL357
	if(receive == 8)
	{
		receive = SPI_ADXL355_Read_Byte(RANGE);
		switch(receive & 0x03)
		{
			case 1: *fsr = 19.5e-6;break; //10g
			case 2: *fsr = 39e-6;break; //20g
			case 3: *fsr = 78e-6;break; //40g
			default:*fsr = 78e-6;break; //40g
		}  
		printf("fsr = %.7f ",*fsr);		
		printf("Set range 40g...\r\n");
	}
	else if(receive == 4)
	{
		receive = SPI_ADXL355_Read_Byte(RANGE);
		switch(receive & 0x03)
		{
			case 1: *fsr = 19.5e-6;break; //10g
			case 2: *fsr = 39e-6;break; //20g
			case 3: *fsr = 78e-6;break; //40g
			default:*fsr = 78e-6;break; //40g
		} 
		printf("fsr = %.7f ",*fsr);		
		printf("Set range 20g...\r\n");
	}
	else if(receive == 2)
	{
		receive = SPI_ADXL355_Read_Byte(RANGE);
		switch(receive & 0x03)
		{
			case 1: *fsr = 19.5e-6;break; //10g
			case 2: *fsr = 39e-6;break; //20g
			case 3: *fsr = 78e-6;break; //40g
			default:*fsr = 78e-6;break; //40g
		} 
		printf("fsr = %.7f ",*fsr);		
		printf("Set range 10g...\r\n");
	}
	else
		printf("Set range error\r\n");
#else  
	if (receive == 8)
	{
		receive = SPI_ADXL355_Read_Byte(RANGE);
		switch(receive & 0x03)
		{
			case 1: *fsr = 3.9e-6;break; //2g
			case 2: *fsr = 7.8e-6;break; //4g
			case 3: *fsr = 15.6e-6;break; //8g
			default:*fsr = 15.6e-6;break; //8g-----0.000156
		}  
		printf("fsr = %.7f ",*fsr);		
		printf("Set range 8g...\r\n");
	}
	else if(receive == 4)
	{
		receive = SPI_ADXL355_Read_Byte(RANGE);
		switch(receive & 0x03)
		{
			case 1: *fsr = 3.9e-6;break; //2g
			case 2: *fsr = 7.8e-6;break; //4g
			case 3: *fsr = 15.6e-6;break; //8g
			default:*fsr = 15.6e-6;break; //8g-----0.000156
		} 
		printf("fsr = %.7f ",*fsr);		
		printf("Set range 4g...\r\n");
	}
	else if(receive == 2)
	{
		receive = SPI_ADXL355_Read_Byte(RANGE);
		switch(receive & 0x03)
		{
			case 1: *fsr = 3.9e-6;break; //2g
			case 2: *fsr = 7.8e-6;break; //4g
			case 3: *fsr = 15.6e-6;break; //8g
			default:*fsr = 15.6e-6;break; //8g-----0.000156
		} 
		printf("fsr = %.7f ",*fsr);		
		printf("Set range 2g...\r\n");
	}
	else
		printf("Set range error\r\n");
#endif 

	//0x02 INT1-FIFO_FULL   0x20INT2-FIFO_FULL  
	SPI_ADXL355_Write_Byte(INT_MAP, 0x02); //开启FIFO-FULL中断 默认会在INT1/INT2引脚上产生一个低电平 读取FIFO或者STATUS寄存器可以清除对应的中断标志位
	receive = SPI_ADXL355_Read_Byte(INT_MAP);
	
	//设置355内部高通滤波器截止频率 
	//可选参数 0x0_-关闭高通滤波器 0x1_-24.7 0x2_-6.2084 0x3_-1.5545 0x4_-0.3862 0x5_-0.0954 0x6_-0.0238  单位：10^-4×ODR
	ADXL355_Set_HPF_Conner(0x00);

	
		//设置数据输出率
	//可选参数 0x_0-4000Hz 0x_1-2000Hz 0x_2-1000Hz 0x_3-500Hz 0x_4-250Hz 0x_5-125Hz 0x_6-62.5Hz 0x_7-31.25Hz 0x_8-15.625Hz 0x_9-7.813Hz 0x_A-3.906Hz 
	ADXL355_Set_Samples(samples);
	samples = ADXL355_Get_Samples();
	ADXL355_Start_Sensor(); 
	HAL_Delay(90);
	
	
	if(samples == 0x02) //1000Hz采集
		printf("1000Hz\r\n");
	else if(samples == 0x05) //125Hz	
	{
		printf("125Hz\r\n");
	}
	else
	{
		printf("4000Hz\r\n");
	}
		
}



/**
   @brief Turns on accelerometer measurement mode.
   @return none
**/
void ADXL355_Start_Sensor(void)
{
	//HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13,GPIO_PIN_RESET); //ADXL355上电
	//HAL_Delay(100);
	
	uint8_t ui8temp;
	ui8temp = SPI_ADXL355_Read_Byte(POWER_CTL);       		/* Read POWER_CTL register, before modifying it */
	ui8temp = ui8temp & 0xFE;                                          /* Set measurement bit in POWER_CTL register */
	SPI_ADXL355_Write_Byte(POWER_CTL, ui8temp);               //xxxxxxx1 - standby模式    xxxxxxx0 - measurement 模式  
	//printf("Sensor started.\n\r");
}


/**
   @brief Puts the accelerometer into standby mode.
   @return none
**/
void ADXL355_Stop_Sensor(void) 
{
	uint8_t ui8temp;
	ui8temp = SPI_ADXL355_Read_Byte(POWER_CTL);       			 /*Read POWER_CTL register, before modifying it */
	ui8temp = ui8temp | 0x01;                                      /* Clear measurement bit in POWER_CTL register */
	SPI_ADXL355_Write_Byte(POWER_CTL, ui8temp);                	 //xxxxxxx1 - standby模式    xxxxxxx0 - measurement 模式  
	//printf("Sensor stopped.\n\r");
}


//新数据产生时 DRDY 引脚电平不再自动跳变
void ADXL355_DRDY_OFF(void)
{
	uint8_t ui8temp;
	ui8temp = SPI_ADXL355_Read_Byte(POWER_CTL);       			 /*Read POWER_CTL register, before modifying it */
	ui8temp = ui8temp | 0x04;                                      /* Clear DRDY_OFF bit in POWER_CTL register */
	SPI_ADXL355_Write_Byte(POWER_CTL, ui8temp);                	 //xxxxx1xx - DRDY_OF    xxxxx0xx - DRDY_ON  
	//printf("\n\rData ready OFF.\n\r");
}


//新数据产生时  DRDY引脚电平自动跳变
void ADXL355_DRDY_ON(void)
{
	uint8_t ui8temp;
	ui8temp = SPI_ADXL355_Read_Byte(POWER_CTL);       			 /*Read POWER_CTL register, before modifying it */
	
	ui8temp = ui8temp & (~0x04);                                       /* Set DRDY_OFF bit in POWER_CTL register */
	SPI_ADXL355_Write_Byte(POWER_CTL, ui8temp);                	 //xxxxx1xx - DRDY_OF    xxxxx0xx - DRDY_ON  
	//printf("\n\rData ready ON.\n\r"); 
}


/**
   @brief Reads the accelerometer data.
   @return none
**/
void ADXL355_Data_Scan(struct ADXL355_Data_int32* acc)  //读取X、Y、Z轴加速度
{
	uint8_t rcv_buffer[3] ={0x00};
	uint8_t *p = (uint8_t*)&ui32SensorX;
		
	SPI_ADXL355_Read_n_Bytes(XDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
		
	p = (uint8_t*)&ui32SensorY;
	SPI_ADXL355_Read_n_Bytes(YDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
		
	p = (uint8_t*)&ui32SensorZ;
	SPI_ADXL355_Read_n_Bytes(ZDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
	
	//printf("%02x %02x %02x\r\n",rcv_buffer[0],rcv_buffer[1],rcv_buffer[2]);
    //ui32SensorT = ADXL355_SPI_Read(TEMP2);
    acc->i32SensorX = ADXL355_Acceleration_Data_Conversion(ui32SensorX);
    acc->i32SensorY = ADXL355_Acceleration_Data_Conversion(ui32SensorY);
    acc->i32SensorZ = ADXL355_Acceleration_Data_Conversion(ui32SensorZ);
	//printf("%d %d %d\r\n", acc->i32SensorX, acc->i32SensorY, acc->i32SensorZ);
}


ADXL355_ST adxl355_read_acc_all(void)
{
	ADXL355_ST acc={0,0,0};
	
	uint8_t rcv_buffer[3] ={0x00};
	uint8_t *p = (uint8_t*)&ui32SensorX;
		
	SPI_ADXL355_Read_n_Bytes(XDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
		
	p = (uint8_t*)&ui32SensorY;
	SPI_ADXL355_Read_n_Bytes(YDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
		
	p = (uint8_t*)&ui32SensorZ;
	SPI_ADXL355_Read_n_Bytes(ZDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
	
	//printf("%02x %02x %02x\r\n",rcv_buffer[0],rcv_buffer[1],rcv_buffer[2]);
    //ui32SensorT = ADXL355_SPI_Read(TEMP2);
    acc.acc_x = ADXL355_Acceleration_Data_Conversion(ui32SensorX);
    acc.acc_y = ADXL355_Acceleration_Data_Conversion(ui32SensorY);
	acc.acc_z = ADXL355_Acceleration_Data_Conversion(ui32SensorZ);
	return acc;
}




int32_t ADXL355_Get_Acc_X(void)  //读取X轴加速度
{
	int32_t tempAccX = 0;
	uint8_t rcv_buffer[3] ={0x00};
	uint8_t *p = (uint8_t*)&tempAccX;
	
    SPI_ADXL355_Read_n_Bytes(XDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
	tempAccX = ADXL355_Acceleration_Data_Conversion(tempAccX);
	return tempAccX;
}


int32_t ADXL355_Get_Acc_Y(void)  //读取Y轴加速度
{
	int32_t tempAccY = 0;
	uint8_t rcv_buffer[3] ={0x00};
	uint8_t *p = (uint8_t*)&tempAccY;
	
    SPI_ADXL355_Read_n_Bytes(YDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
	tempAccY = ADXL355_Acceleration_Data_Conversion(tempAccY);
	return tempAccY;
}

int32_t ADXL355_Get_Acc_Z(void) //读取Z轴加速度
{
	int32_t tempAccZ = 0;
	uint8_t rcv_buffer[3] ={0x00};
	uint8_t *p = (uint8_t*)&tempAccZ;
	
    SPI_ADXL355_Read_n_Bytes(ZDATA3,rcv_buffer,3);
	p[2] = rcv_buffer[0];
	p[1] = rcv_buffer[1];
	p[0] = rcv_buffer[2];
	tempAccZ = ADXL355_Acceleration_Data_Conversion(tempAccZ);
	return tempAccZ;
}

/**
   @brief Convert the two's complement data in X,Y,Z registers to signed integers
   @param ui32SensorData - raw data from register
   @return int32_t - signed integer data
**/
int32_t ADXL355_Acceleration_Data_Conversion (uint32_t ui32SensorData)
{
	int32_t volatile i32Conversion = 0;
	ui32SensorData = ( ui32SensorData >> 4);
	ui32SensorData = (ui32SensorData & 0x000FFFFF);
	if((ui32SensorData & 0x00080000)  == 0x00080000)
	{ //checking if most sig bit is set
         i32Conversion = (ui32SensorData | 0xFFF00000); //if its set, we try to make it negative
	}	
	else
	{
         i32Conversion = ui32SensorData;
	}
	//printf("\n0x%lX\n\r", ui32SensorData);
	return i32Conversion;
}


//设置量程  可选参数 2 4 8 
void ADXL355_Set_Range (uint8_t range) 
{
	uint8_t val = SPI_ADXL355_Read_Byte(RANGE);
	switch(range) 
	{
	case 2 :
		val = (val & 0xFC) | 1;
		break;
	case 4 :
		val = (val & 0xFC) | 2;
		break;
	case 8:
		val = (val & 0xFC) | 3;
		break;
	default :
		printf("\n\rInvalid input - only 2, 4, or 8 for the range\n\r");
		return;
	}
	SPI_ADXL355_Write_Byte(RANGE, val);
}


//设置数据输出率  
//可选参数 0x_0-4000Hz 0x_1-2000Hz 0x_2-1000Hz 0x_3-500Hz 0x_4-250Hz 0x_5-125Hz 0x_6-62.5Hz 0x_7-31.25Hz 0x_8-15.625Hz 0x_9-7.813Hz 0x_A-3.906Hz   
void ADXL355_Set_Samples (uint8_t samples) 
{
	uint8_t val = SPI_ADXL355_Read_Byte(FILTER);
	SPI_ADXL355_Write_Byte(FILTER, val|(samples & 0x0F));
	HAL_Delay(1);
}


//设置高通滤波器截止频率 
//可选参数  0x0_-关闭高通滤波器 0x1_-24.7 0x2_-6.2084 0x3_-1.5545 0x4_-0.3862 0x5_-0.0954 0x6_-0.0238  单位：10^-4×ODR
void ADXL355_Set_HPF_Conner (uint8_t corner) 
{
	uint8_t val = SPI_ADXL355_Read_Byte(FILTER);
	SPI_ADXL355_Write_Byte(FILTER, val|(corner & 0x70));
}



uint8_t ADXL355_Read_Range (void) 
{
	uint8_t range;
	range = SPI_ADXL355_Read_Byte(RANGE);
	range = (range & 0x03);

	if (range == 1)
		return 2;
	else if (range == 2)
		return 4;
	else if (range == 3)
		return 8;
	else return 0; //error
}


uint8_t ADXL355_Get_Samples(void)
{
	uint8_t samples;
	samples = SPI_ADXL355_Read_Byte(FILTER);
	samples =(samples & 0x0F);
	return samples;
}


/*
  函数功能：通过SPI 向 ADXL355 WRITE一个字节的数据
  adress：地址 
  data  ：数据
  写一个字节的数据    
*/
void SPI_ADXL355_Write_Byte(uint8_t regAdress, uint8_t data)
{
	unsigned char writeAdress;                                          
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);             //CS = LOW  片选信号拉低
	writeAdress = regAdress<<1;                                            //寄存器的地址7位 左移1位 右边补0表示写数据  即 xxxxxxx0共8位	 //write adress写入地址
	HAL_SPI_Transmit(&hspi1,&writeAdress,1,10); 						//发送地址
	HAL_SPI_Transmit(&hspi1,&data,1,10);                                          //data to be written写入数据	                           
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);		     //CS = High	 片选信号拉高                              
 }

 
 
 /*
  函数功能：通过SPI 从 ADXL355读取一个字节的数据
  adress：地址 
  读取一个字节的数据    
*/
 uint8_t SPI_ADXL355_Read_Byte(uint8_t regAdress)
{
	uint8_t result;
	unsigned char readAdress;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);    //CS = LOW
	readAdress = regAdress << 1;                                            //寄存器的地址7位 左移1位 
	readAdress |= 0x01;                                                //寄存器的地址7位 最后一位为1表示读取数据 xxxxxxx1
	if(HAL_SPI_Transmit(&hspi1,&readAdress,1,10) != HAL_OK)                            			//read adress读地址
		return 0xFF;
    if(HAL_SPI_Receive(&hspi1, &result,1,10) != HAL_OK)                                    //Read 8bit data (Send Dummy data 0xff)
		return 0xFF;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);      //CS = High	
	return result;
}


 /*
	函数功能：通过SPI 从 ADXL355读取多个个字节的数据
	adress：地址;  rcv_buffer: 接收缓冲区,  n：要读取的字节数 
	读取一个字节的数据    
*/
 uint8_t SPI_ADXL355_Read_n_Bytes(uint8_t regAdress, uint8_t* rcv_buffer, uint16_t n)
{
	unsigned char readAdress;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);    //CS = LOW
	readAdress = regAdress << 1;                                            //寄存器的地址7位 左移1位 
	readAdress |= 0x01;                                                //寄存器的地址7位 最后一位为1表示读取数据 xxxxxxx1
	if(HAL_SPI_Transmit(&hspi1,&readAdress,1,10) != HAL_OK)                            			//read adress读地址
		return SPI_READ_ERROR;
    if(HAL_SPI_Receive(&hspi1,rcv_buffer,n,10) != HAL_OK)                            
		return SPI_READ_ERROR;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);      //CS = High	
	
//	for(int i = 0;i < n; i++)
//		printf("0x%02x \r\n",rcv_buffer[i]);
	
	return SPI_READ_OK;
}


 uint8_t SPI_ADXL355_Read_n_Bytes_DMA(uint8_t regAdress, uint8_t* rcv_buffer, uint16_t n)
{
	unsigned char readAdress;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);    //CS = LOW
	readAdress = regAdress << 1;                                            //寄存器的地址7位 左移1位 
	readAdress |= 0x01;                                                //寄存器的地址7位 最后一位为1表示读取数据 xxxxxxx1
	if(HAL_SPI_Transmit(&hspi1,&readAdress,1,10) != HAL_OK)                            			//read adress读地址
		return SPI_READ_ERROR;
    if(HAL_SPI_Receive_DMA(&hspi1,rcv_buffer,n) != HAL_OK)                            
		return SPI_READ_ERROR;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);      //CS = High	
	
//	for(int i = 0;i < n; i++)
//		printf("0x%02x \r\n",rcv_buffer[i]);
	
	return SPI_READ_OK;
}


void get_adxl355_fifo_data1(struct ADXL355_Data_int32 *target,uint8_t len)
{
	
	//#warning 此函数消耗至少 ADXL355_LEN*9*2byte(若ADXL355_LEN=32,则消耗32*9*2=576byte) RAM 注意RAM是否够用
	uint8_t loop,loop_len,adxl355_max_xyz_len;
	
	uint8_t dat[ADXL355_LEN*3*3]={0};//一组数据分为3个轴，一个轴3个byte数据，共ADXL355_LEN*3*3 byte
	struct ADXL355_Data_int32 buff[ADXL355_LEN] = {{0,0,0}};//缓存整理后的数据
	//先连续读取32组数据,然后再查找哪一组数据是X轴
	SPI_ADXL355_Read_n_Bytes(FIFO_DATA,(uint8_t*)dat,ADXL355_LEN*3*3);
	
	//查找X轴 x轴的 AXIS MARKER为1 EMPTY INDICATOR为0
	loop_len = ADXL355_LEN * 3;//一组数据分为3个轴，共ADXL355_LEN*3个轴
	//printf("loop_len = %d \r\n",loop_len);
	
	
	//查找X轴 x轴的 AXIS MARKER为1 EMPTY INDICATOR为0
	loop_len = ADXL355_LEN * 3;	//一组数据分为3个轴，共ADXL355_LEN*3个轴的数据
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
		//printf("%lf %lf %lf\r\n",((int32_t)buff[loop].i32SensorX)*3.9e-6, ((int32_t)buff[loop].i32SensorY)*3.9e-6,((int32_t)buff[loop].i32SensorZ)*3.9e-6);
	}
	
	//获取数据，fifo,先进先出，最后的最新
	if(adxl355_max_xyz_len==0)//没有有效的数据，返回buff[0],此时buff[0]的值全为0
	{
		printf("adxl355_max_xyz_len==0\r\n");
		for(loop=0;loop<len;loop++)
		{
			target[loop] = buff[0];
		}
	}
	else if(adxl355_max_xyz_len<len)//有效数据数量小于要获取的数据量
	{
		printf("adxl355_max_xyz_len<len\r\n");
		for(loop=0;loop<adxl355_max_xyz_len;loop++)//先填充有效的数据
		{
			target[loop] = buff[loop];
		}
		
		for(loop=adxl355_max_xyz_len;loop<len;loop++)//不够的数据直接填充0
		{
			target[loop].i32SensorX = 0;
			target[loop].i32SensorY = 0;
			target[loop].i32SensorZ = 0;
		}
	}
	else//有效数据数量大于要获取的数据量
	{
		//printf("other case\r\n");
		for(loop=0;loop<len;loop++)
		{
			target[loop] = buff[loop];
		}
	}
}





uint16_t crc16(uint8_t *data,uint8_t len)  
{
	uint16_t crc=0;
	uint8_t i;
	while(len--)
	{
		for(i=0x80;i!=0;i>>=1)
		{
			if((crc&0x8000)!=0)
			{
				crc<<=1;
				crc^=0x1021;
			}
			else crc<<=1;
			
			if((*data&i)!=0)crc^=0x1021;
		}
		data++;
	}
	return crc;
}



