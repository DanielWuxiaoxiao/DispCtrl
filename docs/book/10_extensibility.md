# 第10章 扩展、二次开发与最佳实践

## 新显示模块
- 继承 `QGraphicsItem` 或 `QGraphicsObject`，实现 `paint()`、`boundingRect()`。
- 在 Manager 中注册数据订阅点并使用信号推送到场景。

## 协议扩展
- 在 `Basic/Protocol.h` 中添加新的消息 ID 与结构。  
- 在 UDP 接收逻辑中添加解析分支与对应的信号。  
- 更新 `docs/internal_protocol.md` 与 `docs/changes_*.md`。

## 代码风格建议
- 用 Doxygen 风格注释函数/结构。  
- 遵循项目现有命名与注释约定。  
- 为复杂逻辑增加单元测试。