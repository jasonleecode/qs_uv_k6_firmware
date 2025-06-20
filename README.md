# 本版本固件的改进项

1. 之前的代码组织很乱，重新组织了一下；
2. 重构了makefile文件；
3. 增加了cmake编译文件；

# build
本工程是在ubuntu20.04环境编译，Windows环境还没有测试，暂不提供支持，Mac环境稍后测试；
## dependency
    pip install crcmod
    sudo apt-get install cmake gcc-arm-none-eabi build-essential

## 编译方法
    #方法1
    make

    #方法2
    mkdir build && cd build
    cmake ..
    make -j4

# 硬件特性
## 主控制器 DP32G030
- 32位ARM Cortex M0处理器内核
- 最高工作频率为72MHz
- 24位系统滴答时钟
- 集成嵌套向量中断控制器(NVIC)，提供最多32个中断
- 通过SWD接口烧录程序
- 内置64K字节FLASH存储器作为程序存储区
- 内置16K字节RAM作为数据存储区

## EEPROM BL24C64A
存储容量64K bit，IIC接口通信

## RF BK4819
工作频率范围50MHz ~ 1000MHz，SPI接口通信，支持发射、接收、调制解调、CTCSS/DCS、RSSI检测、音频处理