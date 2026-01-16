

#ifndef ADXL355_H_
#define ADXL355_H_

#include "main.h"
#include "spi.h"

#include "gpio.h"
#include "usart.h"
/********************************* Definitions ********************************/

/* ADXL355 registers addresses */
#define DEVID_AD                 0x00
#define DEVID_MST                0x01
#define PARTID                   0x02
#define REVID                    0x03
#define STATUS                   0x04
#define FIFO_ENTRIES             0x05
#define TEMP2                    0x06
#define TEMP1                    0x07
#define XDATA3                   0x08
#define XDATA2                   0x09
#define XDATA1                   0x0A
#define YDATA3                   0x0B
#define YDATA2                   0x0C
#define YDATA1                   0x0D
#define ZDATA3                   0x0E
#define ZDATA2                   0x0F
#define ZDATA1                   0x10
#define FIFO_DATA                0x11
#define OFFSET_X_H               0x1E
#define OFFSET_X_L               0x1F
#define OFFSET_Y_H               0x20
#define OFFSET_Y_L               0x21
#define OFFSET_Z_H               0x22
#define OFFSET_Z_L               0x23
#define ACT_EN                   0x24
#define ACT_THRESH_H             0x25
#define ACT_THRESH_L             0x26
#define ACT_COUNT                0x27
#define FILTER                   0x28
#define FIFO_SAMPLES             0x29
#define INT_MAP                  0x2A
#define SYNC                     0x2B
#define RANGE                    0x2C
#define POWER_CTL                0x2D
#define SELF_TEST                0x2E
#define ADX_RESET                0x2F
/**************************** Configuration parameters **********************/

/* Temperature parameters */
#define ADXL355_TEMP_BIAS       (float)1852.0      /* Accelerometer temperature bias(in ADC codes) at 25 Deg C */
#define ADXL355_TEMP_SLOPE      (float)-9.05       /* Accelerometer temperature change from datasheet (LSB/degC) */


#define SPI_READ_ERROR 0  //SPI读取数据失败
#define SPI_READ_OK 1    //SPI读取数据成功

/* Accelerometer parameters */
#define ADXL_RANGE     2     /* ADXL362 sensitivity: 2, 4, 8 [g] */
#define ACT_VALUE          50     /* Activity threshold value */
#define INACT_VALUE        50     /* Inactivity threshold value */
#define ACT_TIMER          100    /* Activity timer value in ms */
#define INACT_TIMER        10     /* Inactivity timer value in seconds */



/****************************** Global Data ***********************************/


struct ADXL355_Data_int32
{
	int32_t i32SensorX;
	int32_t i32SensorY;
	int32_t i32SensorZ;
};

typedef struct
{	int32_t acc_x;
	int32_t acc_y;
	int32_t acc_z;
}ADXL355_ST;



struct ADXL355_Data_float32
{
	float32_t f32SensorX;
	float32_t f32SensorY;
	float32_t f32SensorZ;
};

typedef union
{
	struct ADXL355_Data_int32 intg;
	struct ADXL355_Data_float32 flot;
} ADXL355_ACCEL_UNION;


//extern struct ADXL355_Data acc;
extern uint8_t cal_flag; 
extern uint8_t range;
extern float32_t threshold_adxl355;

extern uint32_t triger_count; //模式2 事件触发的次数

//保存存储加速度的内存地址以及 XYZ加速度的组数（每三个轴的加速度为一组）
struct Acc_Data_Pointer_And_Length
{
	float32_t  *i32SensorX;
	float32_t  *i32SensorY;
	float32_t  *i32SensorZ;
	
	int length;
};


extern volatile uint32_t ui32SensorX;
extern volatile uint32_t ui32SensorY;
extern volatile uint32_t ui32SensorZ;
extern volatile uint32_t ui32SensorT;



/*************************** Functions prototypes *****************************/

void ADXL355_Power_On(void); //355上电
void ADXL355_Power_Off(void); //355断电
void ADXL355_Init(void); //初始化函数
void ADXL355_Vibration_Config(float32_t *fsr); //振动模式配置
void ADXL355_Till_Config(void);      //倾角模式配置

void ADXL355_Start_Sensor(void); //开始采集数据
void ADXL355_Stop_Sensor(void);  //停止采集数据

void ADXL355_Data_Scan(struct ADXL355_Data_int32* acc);  	//依次获取X Y Z 轴的加速度数据

ADXL355_ST adxl355_read_acc_all(void); 	//依次获取X Y Z 轴的加速度数据


int32_t ADXL355_Acceleration_Data_Conversion (uint32_t ui32SensorData);  //转换函数
int32_t ADXL355_Get_Acc_X(void); //读取X轴加速度
int32_t ADXL355_Get_Acc_Y(void); //读取Y轴加速度
int32_t ADXL355_Get_Acc_Z(void); //读取Z轴加速度

void ADXL355_DRDY_OFF(void);	//新数据产生时 DRDY引脚电平不再自动跳变
void ADXL355_DRDY_ON(void);		//新数据产生时 DRDY引脚电平自动跳变

//设置高通滤波器的截止频率
//可选参数 0x_0-4000Hz 0x_1-2000Hz 0x_2-1000Hz 0x_3-500Hz 0x_4-250Hz 0x_5-125Hz 0x_6-62.5Hz 0x_7-31.25Hz 0x_8-15.625Hz 0x_9-7.813Hz 0x_A-3.906Hz 
void ADXL355_Set_Samples (uint8_t samples);
uint8_t ADXL355_Get_Samples (void);

//设置高通滤波器截止频率 
//可选参数  0x0_-关闭高通滤波器 0x1_-24.7 0x2_-6.2084 0x3_-1.5545 0x4_-0.3862 0x5_-0.0954 0x6_-0.0238  单位：10^-4×ODR
void ADXL355_Set_HPF_Conner (uint8_t corner);


void ADXL355_Set_Range (uint8_t range); //设置量程
uint8_t ADXL355_Read_Range (void) ;   //读取量程
uint8_t SPI_ADXL355_Read_Byte(uint8_t regAdress);  //读取一个字节
void SPI_ADXL355_Write_Byte(uint8_t regAdress, uint8_t data);  //写入一个字节
uint8_t SPI_ADXL355_Read_n_Bytes(uint8_t regAdress, uint8_t* rcv_buffer, uint16_t n);//读取多个字节



//配置355触发功能
//void ADXL355_triger(float ACT_THRESH, uint8_t full_scale,float32_t *fsr);



//读取FIFO
void get_adxl355_fifo_data1(struct ADXL355_Data_int32 *target,uint8_t len);


void ADXL355_get_till(float* accz_355,float*accy_355,float*accx_355);


//void get_adxl355_fifo_data(int *target,uint8_t len);


//求均值
float32_t average(float32_t *data,int length);

//交换
void swap(float32_t *xp, float32_t *yp);

//交换两个浮点数指针
void swap_float_(float32_t** a, float32_t** b);

//找到最大的峰值及其索引
void findTopPeaks(float *arr, int size, float32_t *topFour, int *indices);

//void findTopPeaks(float32_t *arr, int size, float32_t *topFour, int *index);
//取绝对值
float32_t abs_f(float32_t in);


//内存复制
//void mem_copy(uint8_t* disti, uint8_t* source,size_t size);


//void ADXL355_test_Config(float32_t* fsr);

uint16_t crc16(uint8_t *data,uint8_t len);
#endif /* ADXL355_H_ */
