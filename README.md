# 本版本固件的改进项

1. 之前的代码组织很乱，重新组织了一下；done
2. 重构了makefile文件；done
3. 增加了cmake编译文件；done
4. 增加CW（摩尔斯码）信标发送功能；done
5. 增加rt-thread系统移植；


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

> CW 功能由编译宏 `ENABLE_CW` 控制。`make`（方法1）默认已开启；CMake（方法2）
> 因为全功能镜像已接近 60K Flash 上限，CW 默认关闭，需要时请先腾出空间再开启，例如：
>
>     cmake -DENABLE_CW=ON -DENABLE_SPECTRUM=OFF ..

# CW 摩尔斯码信标
本固件支持发送 CW（摩尔斯码）信标，采用 MCW 方式（在 FM 载波上键控侧音），
接收端在 FM 解调后即可听到标准摩尔斯电码。

## 特性
- 非阻塞、可中断的键控：发送过程中按 **PTT 或任意按键**可立即停止；
- 时序遵循 PARIS 标准（点=1单位、划=3、元素间隔1、字符间隔3、词间隔7）；
- 支持字母 A~Z、数字 0~9 及常用标点；
- 速度、侧音、信标内容均可在菜单中调整并掉电保存。

## 使用方法
1. 菜单里把某个侧键（`F1Shrt`/`F1Long`/`F2Shrt`/`F2Long`/`M Long`）的功能设为
   **`CW BEACON`**，按下该键即发送信标消息；
2. 相关菜单项：
   - `CWSpd`：键控速度，5 ~ 40 WPM；
   - `CWTon`：侧音频率，预设 400/500/600/700/800/900/1000/1200 Hz；
   - `CWMsg`：信标内容（最多 16 字符），机内编辑——数字键输入数字，
     **↑/↓ 滚动字符**，`F` 键输入空格，`*` 键输入 `-`，编辑满后确认保存。

> 注意：发送前需满足正常发射条件（频率合法、电量正常、FM 模式），否则会双哔提示并取消。
> 若本机设置了 BOT 的 PTT-ID，信标前会先发一段 DTMF（按需在菜单关闭 PTT-ID 即可）。

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