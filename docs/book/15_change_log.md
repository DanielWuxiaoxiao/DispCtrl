# 第15章 更新历史（Change Log）

## v1.0 - 基于当前仓库快照
- 对齐协议：BeamControl 结构更新（移除 tranStart，sampleLen -> sampleEnd）。
- UI：waveandsample 频点改为 9.0~9.8 GHz，并实现 index*10 映射。 移除发射起始控件。
- BIT：增强 BITReport 文档及健康对话显示（4x2 按钮网格）。
- 覆盖层：修复 overlay 在 Linux 上的初始化问题（移除固定 geometry，延迟设置几何）。
- 构建：将缺失文件（customspinboxstyle、servocontrol 等）同步到 CMakeLists / DispCtrl.pro。
