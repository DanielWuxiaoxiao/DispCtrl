# 第11章 运维与故障排查手册

## 常见问题与解决方案
- GUI 在 Linux 上控件挤在左上角：检查 `.ui` 是否包含固定 `geometry`，移除并使用布局管理器。  
- 程序退出崩溃（heap assertion）：避免将主窗口作为模态对话的父窗口；使用 `QTimer::singleShot(0, qApp, &QApplication::quit)` 延迟退出。  
- undefined vtable：确认实现 cpp 已被构建系统包含（CMakeLists / DispCtrl.pro）。  
- MOC 未生成：检查 `Q_OBJECT` 是否存在于头文件并列在 HEADERS 中。

## 故障排查步骤
1. 查看日志（`Basic/log` 输出）。
2. 在问题模块增加临时 debug 输出（`qDebug()`）。
3. 使用 tools（如 Valgrind / AddressSanitizer）排查内存问题。
4. 在不同平台复现问题，确认是否为平台差异。