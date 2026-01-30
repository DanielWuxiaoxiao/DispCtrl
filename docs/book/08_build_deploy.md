# 第8章 构建、部署与跨平台注意事项

## 构建（Windows）
- 依赖：Qt 5.14、CMake、MSVC 2017+
- 命令示例：
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Debug
```

## 构建（Linux）
- 依赖：Qt5 (widgets, webengine)、CMake、GCC/Clang
- 常见问题：大小写敏感（确保 CMakeLists.txt 包含所有实现 cpp 文件）、MOC 未生成（确认 HEADERS 列表）。

## 跨平台问题与解决策略
- UI 固定 geometry：在 Linux 上常导致控件在左上角堆叠，应删除固定 geometry 并使用布局管理器。
- overlay 初始化时机：不要在构造函数使用父窗口的 width()/height()，改为使用 `QTimer::singleShot(0, ...)` 延迟设置几何。
- 链接错误（undefined vtable）：检查 CMakeLists 是否包含实现文件（例如 customspinboxstyle.cpp）。

## 部署
- 输出目录需包含 `config.toml`、`resource.qrc` 打包的资源（可用 CMake 自定义目标复制）。
- 建议制作 installer 或 Docker 镜像（Linux 场景）。
