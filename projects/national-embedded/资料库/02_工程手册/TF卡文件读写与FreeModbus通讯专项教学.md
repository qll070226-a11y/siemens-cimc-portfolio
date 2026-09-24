# TF 卡文件读写与 FreeModbus 通讯专项教学

> 适用工程：`CIMC_2026_国赛工程模板`  
> 适用硬件：GD32F470 控制板、TF 卡、USB-RS485、GD30AD3344 采样板  
> 教学目标：不仅会运行模板，还能根据现场题目修改文件格式、寄存器和参数读写行为。

## 1. 先看清国赛新增要求

全国总决赛样题第 1 页明确提出：

- 实现 TF 卡的文件读写功能；
- 实现 Modbus 通讯功能。

第 2 页进一步要求：

- 同时完成多通道数据采集、存储；
- 完成静态参数、定制参数等的读写；
- 利用 Free Modbus 库完成 Modbus 从站开发。

因此，最低合格实现不是“TF 卡能生成一个文件、串口能回一帧”就结束，而应形成下面的闭环：

```text
三通道采样
   ├─→ 测量寄存器（只读） ─→ Modbus 功能码 04 ─→ 上位机
   ├─→ 配置寄存器（读写） ←→ Modbus 功能码 03/06/10
   └─→ CSV/参数文件 ←→ FatFs 文件读写 ←→ SDIO ←→ TF 卡
```

当前模板已经具备：

- SDIO、FatFs 底层和 CSV 追加写；
- FreeModbus RTU 从站、RS485 方向控制、RTU 定时；
- 30001～30006 输入寄存器；
- 40001～40003 保持寄存器；
- Windows 比赛调试上位机。

当前模板仍需你理解的两个边界：

1. CSV 追加写已经完成，但业务层尚未对外提供通用文件读取函数。
2. 40001～40003 可以读写 RAM，但写入后尚未真正改变调度周期，也未掉电保存。

这两点很可能正是现场题目的改题位置。

## 2. 比赛前的最短训练路线

按下面顺序练，不要同时改很多模块：

1. 用现有固件读通 30001，确认 RS485 链路。
2. 连续读取 30001～30006，确认寄存器映射和状态位。
3. 写入并读回 40001～40003，理解保持寄存器。
4. 插入 FAT32 TF 卡，运行 2 分钟，检查 `CIMC_DATA.CSV`。
5. 给业务层增加一个文件读取函数，读回 CSV 表头或参数文件。
6. 让 40002、40003 真正控制记录周期和记录开关。
7. 上位机连续轮询时同时写 TF 卡，运行至少 30 分钟。
8. 测试拔卡、断线、掉电重启、错误波特率和错误从站地址。

完成第 6 步后，才算真正掌握两项新增功能。

---

# 第一部分：TF 卡文件读写

## 3. 不要把 TF 卡功能理解成一个驱动文件

模板中的 TF 卡链路有四层：

```text
contest_storage_final.c       业务层：CSV、参数文件、错误状态
             ↓
ff.c / ff.h                   文件系统层：f_open/f_read/f_write/f_sync
             ↓
diskio.c                      适配层：扇区读写接口
             ↓
sdio_sdcard.c                 硬件层：SDIO、DMA、卡初始化
```

对应文件：

| 层次 | 文件 | 现场通常是否修改 |
|---|---|---|
| 业务层 | `APP/Function/contest_storage_final.c` | 经常修改 |
| 业务接口 | `APP/HeaderFiles/contest_storage.h` | 增加 API 时修改 |
| FatFs | `APP/DeviceLib/FileSystem/ff.c` | 通常不改 |
| FatFs 配置 | `APP/DeviceLib/FileSystem/ffconf.h` | 仅按需求改 |
| 磁盘适配 | `APP/DeviceLib/FileSystem/diskio.c` | 换底层或排障时改 |
| SDIO 驱动 | `APP/DeviceLib/SdCard/sdio_sdcard.c` | 引脚/硬件变化时改 |

现场题若只改变文件名、列顺序、数据格式，不要碰 SDIO 和 FatFs 核心，只改业务层。

## 4. FatFs 的正确操作顺序

### 4.1 初始化和挂载

```c
static FATFS s_fs;

if (disk_initialize(0) != RES_OK) {
    /* SDIO 或卡初始化失败 */
}

if (f_mount(0, &s_fs) != FR_OK) {
    /* 文件系统挂载失败 */
}
```

物理盘号 `0` 对应路径前缀 `0:/`。挂载成功只说明 FatFs 能识别文件系统，不代表某个具体文件一定存在。

### 4.2 创建或覆盖一个文件

```c
FIL file;
FRESULT fr;
UINT written;
static const char text[] = "CIMC TF test\r\n";

fr = f_open(&file, "0:/TEST.TXT", FA_CREATE_ALWAYS | FA_WRITE);
if (fr == FR_OK) {
    fr = f_write(&file, text, sizeof(text) - 1U, &written);
    if ((fr == FR_OK) && (written == sizeof(text) - 1U)) {
        fr = f_sync(&file);
    }
    (void)f_close(&file);
}
```

标志位含义：

| 标志 | 行为 |
|---|---|
| `FA_CREATE_ALWAYS` | 每次清空后重建 |
| `FA_OPEN_ALWAYS` | 不存在则创建，存在则保留 |
| `FA_OPEN_EXISTING` | 只打开已有文件 |
| `FA_READ` | 读取 |
| `FA_WRITE` | 写入 |

模板日志使用 `FA_OPEN_ALWAYS | FA_WRITE`，再 `f_lseek` 到文件末尾，所以是追加而不是覆盖。

### 4.3 追加一行 CSV

核心顺序必须完整：

```c
f_open(..., FA_OPEN_ALWAYS | FA_WRITE);
f_lseek(&file, f_size(&file));
f_write(&file, line, length, &written);
f_sync(&file);
f_close(&file);
```

必须同时检查 `FRESULT` 和 `written`。`f_write` 返回 `FR_OK` 但实际写入字节数不足，仍然属于失败。

`f_sync` 会把缓存尽快落盘，降低突然断电时的数据损失，但它也会增加阻塞时间。当前模板每秒同步一次，适合比赛可靠性优先的场景。

### 4.4 读取一个小文本文件

把下面的函数放到 `contest_storage_final.c`，声明放到 `contest_storage.h`：

```c
int contest_storage_read_text(const char *path,
                              char *buffer,
                              uint32_t capacity,
                              uint32_t *out_length)
{
    FIL file;
    FRESULT fr;
    UINT bytes_read = 0U;

    if (!s_mounted || path == NULL || buffer == NULL || capacity < 2U) {
        return -1;
    }

    fr = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
    if (fr != FR_OK) {
        return -2;
    }

    fr = f_read(&file, buffer, (UINT)(capacity - 1U), &bytes_read);
    buffer[bytes_read] = '\0';

    if (f_close(&file) != FR_OK && fr == FR_OK) {
        fr = FR_DISK_ERR;
    }

    if (out_length != NULL) {
        *out_length = (uint32_t)bytes_read;
    }
    return (fr == FR_OK) ? 0 : -3;
}
```

头文件声明：

```c
int contest_storage_read_text(const char *path,
                              char *buffer,
                              uint32_t capacity,
                              uint32_t *out_length);
```

调用示例：

```c
char text[128];
uint32_t length;

if (contest_storage_read_text("0:/CONFIG.TXT",
                              text, sizeof(text), &length) == 0) {
    /* text 已经补了字符串结束符，可以解析 */
}
```

这个函数适合读取短配置文件，不适合把大型 CSV 一次性装入 RAM。

### 4.5 分块读取大文件

```c
FIL file;
BYTE block[128];
UINT bytes_read;

if (f_open(&file, "0:/CIMC_DATA.CSV", FA_READ) == FR_OK) {
    do {
        if (f_read(&file, block, sizeof(block), &bytes_read) != FR_OK) {
            break;
        }
        /* 在这里处理 bytes_read 个字节，不要假设它是 C 字符串 */
    } while (bytes_read != 0U);
    (void)f_close(&file);
}
```

判断文件结束的标准是 `bytes_read == 0`，不是缓冲区里遇到 `0x00`。

## 5. 比赛最实用的两种文件设计

### 5.1 测量日志 CSV

模板当前文件：`0:/CIMC_DATA.CSV`

```csv
time_ms,current_mA,voltage1_V,voltage2_V,wire_break
1000,4.002,0.998,5.001,0
2000,12.003,4.999,7.501,0
```

优点是电脑可直接查看，答辩证据直观。现场题改变列顺序时，应同时修改表头和数据行，不能只改其中一个。

### 5.2 静态/定制参数文件

推荐使用固定结构文本，便于断网现场手工检查：

```text
sample_ms=100
log_ms=1000
log_enable=1
slave=1
baud=19200
```

实现时分成三步：

1. 上电读取 `CONFIG.TXT`；
2. 对每个参数做范围检查；
3. 全部合法后才覆盖运行参数。

不要让错误文件直接产生 0ms 任务周期、非法从站地址或数组越界。

如果现场明确要求二进制格式，则使用包含版本号和 CRC 的固定结构：

```c
typedef struct {
    uint32_t magic;       /* 固定标识 */
    uint16_t version;
    uint16_t sample_ms;
    uint16_t log_ms;
    uint8_t  log_enable;
    uint8_t  slave;
    uint16_t crc16;
} contest_file_config_t;
```

不要直接把未打包、含指针或编译器相关填充的结构体裸写入文件。

## 6. FatFs 错误码怎么定位

| 错误 | 常见原因 | 检查顺序 |
|---|---|---|
| `FR_NOT_READY` | 卡未插好、底层未初始化 | 卡座、供电、SDIO 初始化 |
| `FR_NO_FILESYSTEM` | 未格式化或格式不支持 | 电脑重新格式化 FAT32 |
| `FR_NO_FILE` | 文件不存在 | 路径、文件名、创建标志 |
| `FR_INVALID_NAME` | 文件名或编码不兼容 | 先改成 8.3 ASCII 名称 |
| `FR_DISK_ERR` | 扇区读写失败 | 供电、接触、DMA、卡质量 |
| `FR_DENIED` | 权限/空间/访问方式问题 | 写保护、容量、打开模式 |

比赛首选经过完整实测的 8GB、16GB 或 32GB FAT32 卡，文件名先用 ASCII，例如 `DATA.CSV`、`CONFIG.TXT`。当前 FatFs 配置的代码页是 437，中文文件名不适合作为现场首选。

## 7. 当前底层有一个必须知道的风险

`APP/DeviceLib/FileSystem/diskio.c` 中存在：

```c
if (cardstate & 0x02000000) {
    while (1) {
    }
}
```

该状态位表示卡处于锁定相关状态。这里无限循环会卡死整个系统，之后 Modbus、采样和界面都不再运行。比赛工程应改成失败返回：

```c
if (cardstate & 0x02000000U) {
    return STA_NOINIT;
}
```

原则：TF 卡失败可以置错误位，但不能拖死整个控制器。

另外，当前 `get_fattime()` 返回 0，因此文件的 FAT 时间戳不可信。日志正文使用 `time_ms` 不受此影响；若现场要求真实日期时间，需要把 RTC 转换成 FatFs 时间格式。

## 8. TF 卡上板测试步骤

### 8.1 准备

1. 备份卡内文件。
2. 电脑格式化为 FAT32。
3. 先保持卡根目录为空。
4. 断电插卡，不建议在尚未实现热插拔状态机时带电拔插。

### 8.2 最小写入测试

1. 烧录模板 APP。
2. 上电等待 10 秒。
3. 断电后取卡插电脑。
4. 检查根目录是否出现 `CIMC_DATA.CSV`。
5. 检查第一行表头和至少 5 行数据。

### 8.3 读回测试

创建 `CONFIG.TXT`：

```text
sample_ms=200
```

调用上一节的 `contest_storage_read_text()`，临时通过调试串口打印 `length` 和内容，或把读取成功状态映射到一个 Modbus 寄存器。验证完成后删除高频调试打印。

### 8.4 异常测试

| 用例 | 操作 | 合格现象 |
|---|---|---|
| 无卡启动 | 不插 TF 卡上电 | TF 错误位置 1，Modbus 仍响应 |
| 卡内无文件 | 删除配置文件 | 返回明确错误，使用默认参数 |
| 配置非法 | `sample_ms=0` | 拒绝应用，系统不失控 |
| 写入中掉电 | 运行后随机断电 | 已同步部分可打开，系统可再次启动 |
| 容量不足 | 使用接近满的测试卡 | 报写入失败，Modbus 仍响应 |
| 连续运行 | 每秒写一次 30 分钟 | 行数持续增加，无死机 |

---

# 第二部分：FreeModbus RTU 通讯

## 9. 先分清 RS485、Modbus RTU 和寄存器业务

这三层不是同一个概念：

```text
RS485：电气层，决定 A/B、方向控制、终端电阻
Modbus RTU：帧格式，决定地址、功能码、CRC、帧间隔
寄存器映射：业务层，决定哪个地址是什么数据、单位和权限
```

模板的通信链路：

```text
USB-RS485 主站
    ↓ 01 04 ... CRC
USART1：PD6 接收
    ↓ 中断
FreeModbus RTU 状态机 + TIMER6 帧间隔
    ↓ eMBRegInputCB / eMBRegHoldingCB
contest_modbus.c 寄存器数组
    ↓
USART1：PD5 发送，PE8 控制 DE/RE
```

当前参数：从站 1、19200 bit/s、8 数据位、无校验、1 停止位。

## 10. 你已经完成的首帧验证

你的实测报文：

```text
TX  01 04 00 00 00 01 31 CA
RX  01 04 02 00 00 B9 30
```

逐字节解释请求：

| 字节 | 含义 |
|---|---|
| `01` | 从站地址 1 |
| `04` | 读输入寄存器 |
| `00 00` | PDU 起始地址 0，即人类编号 30001 |
| `00 01` | 读取 1 个寄存器 |
| `31 CA` | CRC16，低字节在前 |

响应中的 `02` 是数据字节数，`00 00` 是寄存器值，`B9 30` 是 CRC。上位机显示 `TX 1 / RX 1 / ERR 0`，说明完整链路成功。

## 11. 当前寄存器表

### 11.1 输入寄存器，功能码 04，只读

| PDU 地址 | 人类编号 | 含义 | 单位/换算 |
|---:|---:|---|---|
| 0 | 30001 | 电流 | 原值 ÷ 1000 = mA |
| 1 | 30002 | 电压 1 | 原值 ÷ 1000 = V |
| 2 | 30003 | 电压 2 | 原值 ÷ 1000 = V |
| 3 | 30004 | 状态字 | 位定义见下 |
| 4 | 30005 | 采样计数高 16 位 | 与 30006 组合 |
| 5 | 30006 | 采样计数低 16 位 | 与 30005 组合 |

状态字：

| 位 | 掩码 | 1 的含义 |
|---:|---:|---|
| bit0 | `0x0001` | 4～20mA 断线 |
| bit1 | `0x0002` | TF 卡已就绪 |
| bit2 | `0x0004` | TF 卡发生错误 |

采样计数还原：

```c
uint32_t count = ((uint32_t)reg_30005 << 16) | reg_30006;
```

### 11.2 保持寄存器，功能码 03/06/10，可读写

| PDU 地址 | 人类编号 | 当前定义 | 默认值 |
|---:|---:|---|---:|
| 0 | 40001 | 采样周期 ms | 100 |
| 1 | 40002 | TF 记录周期 ms | 1000 |
| 2 | 40003 | TF 记录使能 | 1 |

重要：当前源码只会修改 `s_holding[]`。写 40002 后，调度器仍使用编译时的 `CONTEST_LOG_PERIOD_MS`；写 40003 后，`contest_storage_task()` 仍由编译宏决定是否记录。读回成功只证明 RAM 寄存器已改，不代表业务已经生效。

## 12. 上位机怎么测试 Modbus

### 12.1 连续读六个测量寄存器

在“通用寄存器”页设置：

```text
类型：输入寄存器
功能码：04
起始地址：0
数量：6
显示：DEC
```

原始请求应为：

```text
01 04 00 00 00 06 70 08
```

采样计数应持续增加。若 30001～30003 为 0 但计数增加，优先检查 ADC 和换算，不要再查 Modbus。

### 12.2 读三个保持寄存器

```text
类型：保持寄存器
功能码：03
起始地址：0
数量：3
```

请求：

```text
01 03 00 00 00 03 05 CB
```

默认响应数据应代表 `100、1000、1`。

### 12.3 写单个保持寄存器

向 40001 写 100，对应请求：

```text
01 06 00 00 00 64 88 21
```

功能码 06 的正常响应会原样回显请求。写完后立即用功能码 03 读回。

### 12.4 一次写三个保持寄存器

写入 `100、1000、1`：

```text
01 10 00 00 00 03 06 00 64 03 E8 00 01 D6 F8
```

其中 `00 03` 是寄存器数量，`06` 是后续数据字节数。CRC 仍然低字节在前。

## 13. FreeModbus 回调为什么要地址减 1

模板的输入寄存器回调：

```c
index = (USHORT)(address - 1U);
```

这是因为 FreeModbus 核心调用业务回调时使用 1 起始内部地址，而报文中的 PDU 地址从 0 开始。主站读 PDU 0，回调收到的 `address` 是 1，减 1 后正好访问数组索引 0。

不要为了“看起来直观”删掉 `-1`，否则所有寄存器都会错位。调试时同时写清：

```text
PDU 地址 0 = 人类编号 30001 = 回调地址 1 = 数组索引 0
```

## 14. 数据字节序和缩放

每个 Modbus 寄存器是 16 位，线上高字节先发：

```c
*buffer++ = (UCHAR)(value >> 8);
*buffer++ = (UCHAR)value;
```

浮点测量值不直接发送，而是缩放成整数：

```text
12.345 mA → 12345 → 0x3039
5.000 V   → 5000  → 0x1388
```

这样能避免不同平台浮点格式、字节序和解析方式不一致。寄存器表必须明确单位与倍率，否则“报文正确、数值错误”仍会失分。

## 15. 如何新增一个只读测量寄存器

假设现场要求增加“设备运行时间秒数”：

第一步，在 `contest_modbus.h` 增加枚举：

```c
enum {
    MB_INPUT_CURRENT_MA_X1000 = 0,
    MB_INPUT_VOLTAGE1_MV      = 1,
    MB_INPUT_VOLTAGE2_MV      = 2,
    MB_INPUT_STATUS           = 3,
    MB_INPUT_SAMPLE_COUNT_HI  = 4,
    MB_INPUT_SAMPLE_COUNT_LO  = 5,
    MB_INPUT_UPTIME_S         = 6,
    MB_INPUT_REGISTER_COUNT   = 16
};
```

第二步，在 `contest_modbus_update_measurements()` 填值：

```c
s_input[MB_INPUT_UPTIME_S] =
    (uint16_t)((get_system_ms() / 1000U) & 0xFFFFU);
```

第三步，用功能码 04 从起始地址 0 读 7 个寄存器。现有回调按数组连续返回，无需再写一套协议解析。

## 16. 如何让保持寄存器真正控制业务

### 16.1 先限制写入范围

建议范围：

```text
40001 采样周期：10～5000 ms
40002 记录周期：100～60000 ms
40003 记录使能：0 或 1
```

在保持寄存器写回调中，先组合 16 位值，再验证：

```c
USHORT value = (USHORT)(((USHORT)buffer[0] << 8) | buffer[1]);

if (index == MB_HOLD_SAMPLE_PERIOD_MS &&
    (value < 10U || value > 5000U)) {
    return MB_EINVAL;
}
if (index == MB_HOLD_LOG_PERIOD_MS &&
    (value < 100U || value > 60000U)) {
    return MB_EINVAL;
}
if (index == MB_HOLD_LOG_ENABLE && value > 1U) {
    return MB_EINVAL;
}
s_holding[index] = value;
```

批量写多个寄存器时，严谨做法是先把所有新值放入临时数组并全部验证，全部合法后再一次提交，避免前几个已写、后一个失败的半更新状态。

### 16.2 给业务层提供读取接口

```c
uint16_t contest_modbus_get_sample_period_ms(void)
{
    return s_holding[MB_HOLD_SAMPLE_PERIOD_MS];
}

uint16_t contest_modbus_get_log_period_ms(void)
{
    return s_holding[MB_HOLD_LOG_PERIOD_MS];
}

uint8_t contest_modbus_get_log_enable(void)
{
    return (uint8_t)(s_holding[MB_HOLD_LOG_ENABLE] != 0U);
}
```

### 16.3 让调度周期动态生效

在调度器每轮执行前更新对应任务的周期，或提供专门的调度配置接口。最小改法是：

```c
s_tasks[1].period_ms = contest_modbus_get_sample_period_ms();
s_tasks[2].period_ms = contest_modbus_get_log_period_ms();
```

并在 `contest_storage_task()` 开头加入：

```c
if (!contest_modbus_get_log_enable()) {
    return;
}
```

不要从中断直接执行文件操作或重配完整业务。Modbus 回调只更新经过验证的参数，主循环任务再应用参数，系统更稳定。

### 16.4 掉电保存

如果正式题要求“静态参数掉电不丢失”，仅用保持寄存器不够。可选方案：

- 保存到 TF 卡 `CONFIG.TXT`；
- 保存到内部 Flash 参数区；
- 使用板载非易失存储器。

推荐工作流：

```text
Modbus 写参数
  ↓ 范围检查
更新 RAM 运行参数
  ↓ 设置 config_dirty
主循环低频执行保存
  ↓ 写临时文件 CONFIG.TMP + f_sync
校验成功
  ↓ 替换 CONFIG.TXT
```

不要在串口接收中断或 FreeModbus 字节回调里擦写 Flash/TF 卡。

## 17. FreeModbus 端口层在做什么

`contest_port_clocked.c` 已完成三项关键适配：

1. USART1 收发字节；
2. PE8 控制 RS485 收发方向；
3. TIMER6 产生 RTU 帧间隔定时。

接收路径：

```text
USART RBNE 中断
→ contest_modbus_rx_isr()
→ pxMBFrameCBByteReceived()
→ xMBPortSerialGetByte()
```

发送路径：

```text
FreeModbus 请求发送
→ PE8 进入发送状态
→ USART TBE 中断逐字节发送
→ 等待 TC 完成
→ PE8 回到接收状态
```

如果能收到请求但没有响应，优先看从站地址、CRC 和回调范围；如果逻辑分析仪看到 MCU TX 有数据但总线没有，优先看 PE8 和 RS485 收发器。

## 18. Modbus 故障树

### 18.1 完全无 RX

依次检查：

1. COM 口是否正确；
2. 波特率、校验、停止位；
3. A/B 是否接反；
4. USB-RS485 是否需要共地；
5. 开发板是否运行 APP；
6. 从站地址是否为 1；
7. 上位机发送是否有 TX 计数。

### 18.2 有 TX，无响应

1. 用原始报文确认 CRC；
2. 请求地址不能是广播地址 0；
3. 功能码必须与寄存器类型匹配；
4. 请求范围不能超过已实现寄存器；
5. 检查 USART 接收中断是否进入；
6. 检查 TIMER6 中断是否进入；
7. 检查 PE8 是否保持在接收方向。

### 18.3 有响应但数值不对

1. 先看十六进制原始值；
2. 核对 PDU 地址和 30001/40001 人类编号；
3. 核对倍率 1000；
4. 核对有符号/无符号；
5. 核对 32 位高低寄存器顺序；
6. 最后再查 ADC 标定。

### 18.4 偶发 CRC 错误

1. 缩短临时测试线；
2. 让 A/B 双绞；
3. 总线两端按需要加 120Ω，不能每个节点都加；
4. 检查偏置和共地；
5. 检查 PE8 切换是否过早；
6. 移除高频串口打印；
7. 检查 TF 卡同步写是否造成过长阻塞。

---

# 第三部分：两项功能同时验收

## 19. 为什么必须做并发测试

当前工程是无 RTOS 的合作式调度。`f_write()`、`f_sync()` 或某个底层等待过久，会推迟 1ms 的 `eMBPoll()`。单独测试 TF 和单独测试 Modbus 都成功，不代表一起运行仍然稳定。

正式验收前必须做：

```text
上位机持续轮询 30001～30006
           +
TF 卡每秒追加一行并同步
           +
三路信号不断变化
```

## 20. 30 分钟联合测试

### 20.1 设置

- Modbus：COM6、19200、None、8N1、从站 1；
- 轮询周期：先 200ms，稳定后试 100ms；
- TF 记录周期：1000ms；
- 三路输入：至少改变 3 个工作点；
- 上位机开启日志保存。

### 20.2 通过标准

| 项目 | 通过标准 |
|---|---|
| Modbus | TX 与 RX 持续增长，ERR 为 0 或无持续错误 |
| 测量 | 三路寄存器随信号变化，倍率正确 |
| 计数 | 30005/30006 持续增长，无长时间停顿 |
| TF 状态 | 30004 bit1 为 1，bit2 为 0 |
| CSV | 行数持续增加，列数固定，无乱码/截断行 |
| 重启 | 断电重启后可继续工作，文件可再次打开 |

理论上每秒一行运行 30 分钟，应新增约 1800 行。允许启动和断电边界产生少量差异，但不能大量缺行。

## 21. 现场改题时如何快速落地

拿到正式题后先填表：

| 题目变化 | 主要修改点 | 验证方式 |
|---|---|---|
| 文件名变化 | `contest_config.h` 路径宏 | 电脑查看根目录 |
| CSV 列变化 | `contest_storage_final.c` 表头和格式 | 对照题目逐列检查 |
| 要求读参数文件 | 增加 `f_read` 和解析/校验 | 写已知文件后读回 |
| 寄存器地址变化 | `contest_modbus.h` 映射 | 原始报文逐地址读 |
| 倍率变化 | `contest_modbus_update_measurements()` | 施加已知值比对 |
| 参数可写生效 | 保持回调 + 业务 getter | 写入、读回、观察行为 |
| 参数掉电保存 | TF/Flash 保存与上电加载 | 写入后断电重启 |
| 波特率/地址变化 | `contest_config.h` | 改上位机后通信 |

每做一个修改，按“编译 → 烧录 → 单功能 → 联合运行”验证。不要积累五处修改后才第一次编译。

## 22. 你现在就可以完成的练习

### 练习 A：TF 写入

目标：卡上出现 CSV，数据每秒增长。

证据：保留 CSV 文件和一张电脑打开后的截图。

### 练习 B：TF 读取

目标：读取 `CONFIG.TXT` 的第一行，并通过调试串口或一个新输入寄存器输出解析结果。

证据：错误文件被拒绝，合法文件能应用。

### 练习 C：Modbus 测量

目标：一次读 30001～30006，解释每个值。

证据：保存 TX/RX 日志，并记录三路输入的基准值。

### 练习 D：Modbus 参数

目标：用功能码 06 修改记录开关，用功能码 03 读回，并让 TF 实际停止/恢复写入。

证据：写 0 后 CSV 行数不再增长，写 1 后恢复。

### 练习 E：掉电保持

目标：参数写入后断电重启仍保持。

证据：重启前后读取 40001～40003，值一致。

### 练习 F：联合压力测试

目标：连续轮询与 TF 写入同时运行 30 分钟。

证据：上位机 ERR、CSV 行数、状态寄存器、重启结果。

## 23. 最终自检清单

- [ ] 能解释 SDIO、diskio、FatFs、业务层四层关系。
- [ ] 能独立写出 `f_open → f_write/f_read → f_sync → f_close`。
- [ ] 能区分覆盖写、追加写和只读打开。
- [ ] 无卡或坏卡不会让主程序死循环。
- [ ] 能解释 RS485、Modbus RTU、寄存器映射的区别。
- [ ] 能手工解释 `01 04 00 00 00 01 31 CA`。
- [ ] 能区分 PDU 地址 0、人类编号 30001、回调地址 1。
- [ ] 能新增一个输入寄存器并通过 04 读出。
- [ ] 能写保持寄存器，并证明它对实际业务生效。
- [ ] 能实现非法参数范围检查。
- [ ] 若题目要求，能实现掉电保存和上电恢复。
- [ ] TF 记录和 Modbus 轮询同时运行 30 分钟无持续错误。

## 24. 建议的答辩表述

TF 卡部分：

> 系统采用 SDIO 与 TF 卡通信，通过 diskio 适配 FatFs。测量数据以 CSV 追加写入，每次校验实际写入长度并同步落盘；配置文件采用读取、范围校验、整体应用流程。存储异常只上报状态，不阻塞采样和 Modbus。

Modbus 部分：

> 系统基于指定 FreeModbus 库实现 RTU 从站。USART1 完成字节收发，PE8 控制 RS485 方向，TIMER6 提供 RTU 帧间隔。测量值使用输入寄存器只读发布，配置参数使用保持寄存器读写，并进行范围校验和掉电保存。

联合可靠性部分：

> 系统采用非阻塞主循环调度，已在连续 Modbus 轮询、三通道采样和 TF 周期写入同时运行的条件下验证。TF 异常通过状态寄存器上报，不影响通信主链路。

---

关联资料：

- `03_专题速查/TF卡与FatFs速查.md`
- `03_专题速查/FreeModbus_RTU速查.md`
- `03_专题速查/RS485与RTU时序.md`
- `02_工程手册/国赛从零到实战教学.md`
- `04_现场表单/功能验收记录表.md`

