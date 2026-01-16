# 第14章 附录

## 协议字段摘录（示例）
- BeamControl:
  - freqID: 协议值为 0~80（UI 以 index*10 显示）
  - sampleEnd: 采样结束时间（单位见协议注释）

## 示例 `config.toml`
见项目根目录下 `config.toml`，并对常用字段做了示例注释。

## 常用命令
- 配置与构建：`cmake ..` / `cmake --build .`  
- 运行：`./build/bin/Debug/DispCtrl.exe`  
- 清理：`git clean -fd`（慎用）