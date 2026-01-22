# ADXL355 DMA 双缓冲实现说明

## 功能概述

实现了 SPI DMA 读取 ADXL355 传感器数据后，自动通过 Memory-to-Memory DMA 将数据拷贝到大缓冲区的功能。

## 工作流程

1. **PB7 外部中断触发** → 调用 `ADXL355_DMA_Start()`
2. **SPI DMA 传输** → 发送读命令 + 接收 288 字节数据到 `adxl_rx_buf`
3. **SPI 完成回调** → `HAL_SPI_TxRxCpltCallback()` 中触发 Memory-to-Memory DMA
4. **Memory DMA 拷贝** → 将 `adxl_rx_buf[1..288]` 拷贝到 `adxl_large_buffer[write_index]`
5. **Memory DMA 完成** → `HAL_DMA_MemCplt_Callback()` 更新索引并打印提示

## 主要配置

### DMA 通道分配
- **GPDMA1_Channel0**: SPI1 RX (Periph → Memory)
- **GPDMA1_Channel1**: SPI1 TX (Memory → Periph)
- **GPDMA1_Channel2**: Memory-to-Memory (Memory → Memory)

### 缓冲区
- `adxl_rx_buf[289]`: SPI 接收缓冲区（第 0 字节为回显命令，1-288 为有效数据）
- `adxl_large_buffer[10000]`: 大缓冲区，存储多次采样数据
- `adxl_write_index`: 当前写入位置

## 使用方法

### 初始化
```c
// 在 main.c 中，初始化后调用
ADXL355_DMA_Read_Prep(FIFO_DATA);  // 准备读 FIFO 的命令
```

### 查询缓冲区状态
```c
uint16_t count = ADXL355_Get_Buffer_Count();  // 获取已存储的字节数
printf("Buffer has %d bytes\n", count);
```

### 读取并处理数据
```c
uint8_t process_buf[10000];
uint16_t len;
ADXL355_Read_Buffer(process_buf, &len);  // 读取所有数据并清空缓冲区
// 处理 process_buf 中的 len 字节数据
```

## 关键参数调整

### 缓冲区大小
在 [spi.c](d:\Myself\Wisen\code\SPI_DMA\SPI_DMAtest\Core\Src\spi.c) 中修改：
```c
#define ADXL_BUFFER_SIZE 10000  // 根据需要调整
```

### 缓冲区满时的处理
在 `HAL_DMA_MemCplt_Callback()` 中：
```c
if (adxl_write_index >= ADXL_BUFFER_SIZE - ADXL_RX_LEN)
{
    // 选项 1: 停止采样，等待读取
    // 选项 2: 循环覆盖（adxl_write_index = 0）
    // 选项 3: 触发数据处理任务
}
```

## 性能优势

1. **CPU 零参与**: 数据从 SPI → 临时缓冲 → 大缓冲全程由 DMA 完成
2. **低延迟**: Memory-to-Memory DMA 速度快（289 字节约 <10 µs）
3. **连续采样**: 支持高频连续采样而不丢数据

## 注意事项

1. **缓存一致性**: 如果开启 D-Cache，需在 DMA 前后处理缓存
2. **缓冲区溢出**: 确保及时读取数据，避免 `adxl_write_index` 超限
3. **中断优先级**: 确保 GPDMA Channel 2 中断优先级合理（当前设为 5）
4. **内存对齐**: 缓冲区已按 4 字节对齐，保证 DMA 效率

## 调试提示

- 编译后可看到打印 "MemCopy Done, index=xxx"，表示每次拷贝完成
- 观察 `adxl_write_index` 的增长来验证采样频率
- 当缓冲区满时会打印 "Buffer Full!"
