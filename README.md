# 西门子杯智能制造挑战赛：工业嵌入式采集与边缘通信系统

> 面向科研导师/工程团队的项目作品集。项目来自西门子杯全国智能制造挑战赛工业嵌入式系统开发方向，重点展示工业模拟量采集、嵌入式软件架构、通信协议、数据记录、Bootloader/OTA 与工程验证能力。

## 项目概览

这是一个以 **GD32F470VET6** 为主控的工业现场边缘采集系统。系统面向两路 `0~10 V` 电压和一路 `4~20 mA` 电流信号，完成隔离采样、工程量换算、断线判断、Modbus RTU 通信、TF 卡 CSV 记录、上位机监控以及 Bootloader 在线升级链路。

仓库以 2026 国赛工程模板为主线，同时保留初赛 OTA 工程的核心源码与工程说明，按“源码可读、架构可讲、验证可追溯”的方式整理，适合用于科研导师面试、嵌入式/控制/智能制造方向作品展示。

## 我的工程能力画像

- **嵌入式软件**：裸机协作式调度、C 语言模块化、外设初始化、中断与超时处理。
- **工业通信**：FreeModbus RTU、RS485 收发方向控制、寄存器映射、CRC/异常码、上位机轮询。
- **数据采集**：GD30AD3344 多通道 SPI 采样、量程/增益/零点标定、断线迟滞与状态位设计。
- **可靠性工程**：Bootloader 分区、固件校验、升级回滚、TF 卡错误状态、构建基线和验收矩阵。
- **工程交付**：Keil 工程、上位机源码、离线资料库、测试表单、构建/审计脚本和可复核文档。

## 技术栈

| 层次 | 技术/组件 | 作用 |
|---|---|---|
| MCU | GD32F470VET6 | 主控、定时调度、外设与中断 |
| 工具链 | Keil MDK、ARM Compiler 6、GigaDevice GD32F4xx DFP | APP/Bootloader 构建与烧录 |
| 语言 | C、C#、PowerShell、Python | 固件、上位机、构建与审计工具 |
| 实时模型 | 裸机 cooperative scheduler（无 RTOS） | 1 ms 通信、100 ms 采样、1 s 日志等周期任务 |
| 采样 | GD30AD3344、SPI、片内 ADC、RC/隔离前端 | 三通道工业模拟量采集 |
| 协议 | Modbus RTU、RS485、FreeModbus | 工业现场读写和状态发布 |
| 存储 | FatFs、TF 卡、CSV | 断网场景下的本地数据留存 |
| 升级 | Bootloader、Flash 分区、CRC、OTA 包 | 固件接收、校验、备份和回滚 |
| 上位机 | .NET Framework 4.8、串口、实时曲线、CSV/PNG | 监控、抓包、数据导出与回放 |
| 工程验证 | 构建日志、SHA256、标定表、需求-代码-验证矩阵 | 让功能结论可追溯、可复核 |

## 系统架构

```text
┌────────────────────────────── 上位机 / 现场工具 ──────────────────────────────┐
│  .NET Framework 4.8  │  Modbus 轮询  │  实时曲线  │  CSV/PNG 导出  │  OTA 文件发送 │
└───────────────────────────────┬──────────────────────────────────────────────┘
                                │ RS485 / Modbus RTU
┌───────────────────────────────▼──────────────────────────────────────────────┐
│                         GD32F470VET6 裸机应用层                               │
│  任务调度 → 采样任务 → 工程量换算/断线状态 → 数据快照                         │
│      │              │                         ├─ Modbus 寄存器发布             │
│      │              │                         └─ TF/FatFs CSV 追加             │
│      └─ UI/告警/系统服务                                                        │
├──────────────────────────────────────────────────────────────────────────────┤
│  FreeModbus  │  USART/RS485 DE-RE  │  SPI  │  GD30AD3344  │  FatFs  │  RTC/UI │
└───────────────────────────────┬──────────────────────────────────────────────┘
                                │ Bootloader 跳转 / 固件升级
┌───────────────────────────────▼──────────────────────────────────────────────┐
│  Bootloader：接收 → 分片写入 → CRC/魔术字校验 → APP 备份 → 跳转 / 回滚           │
└──────────────────────────────────────────────────────────────────────────────┘
```

## 软件架构与关键模块

### 1. 应用层数据链路

```text
GD30AD3344 CH4/CH5/CH6
        ↓
contest_adc.c：ADC 端电压
        ↓
contest_app.c：增益/零点换算、断线与状态
        ├── contest_modbus.c：输入/保持寄存器
        └── contest_storage_final.c：CIMC_DATA.CSV
```

主循环使用轻量合作式调度器，避免引入 RTOS 的额外资源和不确定性。当前基线的典型周期为：Modbus `1 ms`、采样 `100 ms`、UI `50 ms`、TF 日志 `1000 ms`。所有任务共享业务数据快照，协议层不直接操作 ADC 或文件系统。

### 2. 通信与寄存器

- 默认从站地址 `1`，`19200 / 8N1`。
- 输入寄存器发布电流、电压 1、电压 2、状态位和采样计数。
- 状态位示例：bit0 电流断线、bit1 TF ready、bit2 TF error。
- FreeModbus 负责 RTU 状态机、帧间隔和异常响应，板级端口负责 USART、定时器和 RS485 DE/RE。

### 3. 采样与标定

软件采用 `工程量 = ADC 端电压 × gain + offset` 的线性模型，电流通道额外支持低阈值断线判断。资料库给出了多点标定、误差定义、串扰检查和边界测试方法；默认增益仅是模板占位值，未完成实板标定前不宣称精度指标。

### 4. Bootloader / OTA

Bootloader 独立于 APP 构建，负责升级接收、分片写入、固件头/CRC 校验、APP 备份与失败恢复。仓库同时收录初赛工程中更完整的 OTA 包格式说明，便于对比分区设计、包头字段和回滚策略。

## 仓库结构

```text
.
├── README.md                         # 项目总览、技术栈、架构与面试入口
├── docs/
│   ├── public-scope.md               # 公开范围、排除项与版权/隐私边界
│   ├── source-inventory.md           # 原始资料盘点与公开版映射
│   └── architecture-notes.md         # 面向导师的架构与验证摘要
├── projects/
│   ├── national-embedded/            # 2026 国赛：APP + Bootloader + 资料库 + 上位机
│   └── preliminary-ota/              # 初赛：GD32F470 APP/OTA 核心源码与 Keil 工程
└── tools/
    └── README.md                     # 本公开版的复核与后续整理建议
```

## 快速阅读路线

1. 先读本文的“技术栈”和“系统架构”，了解完整数据流。
2. 阅读 [`projects/national-embedded/资料库/02_工程手册/国赛工程完整使用手册.md`](projects/national-embedded/资料库/02_工程手册/国赛工程完整使用手册.md)，查看实现边界、周期、寄存器和 Bootloader 分区。
3. 阅读 [`projects/national-embedded/资料库/00_导航/需求代码验证矩阵.md`](projects/national-embedded/资料库/00_导航/需求代码验证矩阵.md)，按需求定位源码和验证证据。
4. 查看 [`projects/national-embedded/APP/`](projects/national-embedded/APP/) 与 [`projects/national-embedded/BootLoader/`](projects/national-embedded/BootLoader/)，对应应用层和升级链路实现。
5. 阅读 [`projects/preliminary-ota/01_Readme/README.md`](projects/preliminary-ota/01_Readme/README.md)，对比初赛 OTA 包格式与 Flash 分区设计。

## 构建与复现

### 国赛工程

1. 安装 Keil MDK、ARM Compiler 6 和对应 GD32F4xx Device Pack。
2. 打开 `projects/national-embedded/APP/MDK/CIMC_APP.uvprojx`，执行 Rebuild。
3. 打开 `projects/national-embedded/BootLoader/MDK/CIMC_BL.uvprojx`，执行 Rebuild。
4. 按资料库中的构建基线与复核说明检查 `0 Error(s), 0 Warning(s)`、HEX/BIN 和 SHA256。

### 初赛 OTA 工程

打开 `projects/preliminary-ota/project/Project.uvprojx`。工程说明位于 `01_Readme/README.md`，其中记录 APP1 地址、OTA 头字段、分片发送方式和回滚区域。

> 公开仓库没有携带 Keil、GD32 Pack、安装包或编译输出；这些属于环境依赖或生成物，应在本地按工程文件配置。

## 验证与诚实边界

资料库已经整理了构建检查、Modbus 抓包、三通道标定、TF 卡并发、异常地址、长稳测试和提交前验收流程。当前公开版仍应把以下内容视为“需要目标硬件复测”的项目：ADC 实际精度、隔离性能、RS485 电气方向、TF 卡兼容性以及 Bootloader 在真实板卡上的擦写/回滚。

面试展示时建议按“需求 → 设计 → 代码位置 → 验证证据 → 已知限制”的顺序讲解，不把“能编译”表述成“已完成硬件验证”。

## 公开范围与许可

本仓库是从个人资料合集整理出的研究/求职展示版，不代表西门子官方发布，也不替代正式赛题、裁判说明或官方 SDK。第三方库、竞赛原文和硬件厂商资料保留其原有版权与许可；仓库不对第三方资料授予额外许可。详细边界见 [`docs/public-scope.md`](docs/public-scope.md)。

