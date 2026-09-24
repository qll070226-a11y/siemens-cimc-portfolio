# 原始资料与公开版映射

## 原始资料盘点

原始目录：`D:\西门子比赛资料合集\西门子杯比赛资料合集`

| 模块 | 原始规模 | 公开版处理 |
|---|---:|---|
| `西门子国赛` | 约 1.7 GB / 12,027 文件 | 以 `CIMC_2026_国赛工程模板` 为主线，保留源码、工程、脚本和 Markdown |
| `西门子/2026年CIMC工业嵌入式系统开发 初赛 赛题` | 约 2.2 GB / 20,720 文件 | 保留 `MICU_GD_APP` 的核心源码、Keil 工程和 OTA 说明 |
| `2026931391` | 约 0.9 GB / 2,427 文件 | 未直接复制；其 APP/Bootloader 内容与公开国赛工程存在重复，视频和构建输出不公开 |
| `嵌入式决赛资料2026` | 约 7 MB / 25 文件 | 作为现场资料索引保留在原始合集，不纳入源码仓库 |
| `西门子演示视频.zip` | 约 2.3 GB | 不纳入 GitHub；GitHub 普通仓库不适合存放此类大文件 |

原始合集总量约 11 GB，公开版约 25 MB。公开版体积小不是资料丢失，而是主动排除了视频、安装包、重复备份、编译产物、数据库和无法确认再分发许可的压缩包。

## 公开版目录映射

| 公开目录 | 内容 | 适合阅读的入口 |
|---|---|---|
| `projects/national-embedded/APP` | 国赛 APP、GD32 外设、采样、协议、存储和 UI | `APP/MDK/CIMC_APP.uvprojx` |
| `projects/national-embedded/BootLoader` | Bootloader、Flash 分区和升级相关源码 | `BootLoader/MDK/CIMC_BL.uvprojx` |
| `projects/national-embedded/资料库` | 导航、工程手册、专题速查、现场表单 | `资料库/README.md` |
| `projects/national-embedded/上位机` | .NET Framework 上位机源码和协议测试代码 | `上位机/README.md` |
| `projects/preliminary-ota` | 初赛 GD32F470 APP/OTA 核心工程 | `01_Readme/README.md` |

## 未纳入文件类型

`.mp4`、`.zip`、`.7z`、`.rar`、`.exe`、`.msi`、`.dll`、`.db`、`.sqlite`、`.o`、`.axf`、`.map`、`.crf`、`.d`、`.dep`、`.hex`、`.bin` 以及超过 100 MB 的单体文档均未复制。

## 建议

如需向导师展示视频或完整离线资料，建议通过学校网盘、对象存储或现场演示提供，并在 GitHub README 中只放经过授权的链接；不要把竞赛原始资料和第三方安装包直接加入公开 Git 历史。
