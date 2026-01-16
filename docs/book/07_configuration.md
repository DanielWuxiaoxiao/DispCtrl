# 第7章 配置说明（config.toml）

`config.toml` 包含应用启动时所需的关键配置。本章对常用字段进行解释与示例。

## 网络配置示例
```toml
[network.ports]
DATA_PRO_2_DISP = 8007
SIG_2_DISP_PORT1 = 8008
TAR_2_DISP = 8011
EXT_SYSCTRL_SRC = 6001
EXT_SYSCTRL_DST = 8001
EXT_ACK_DST = 8002
```

## 显示与地图
```toml
[polarDisp.range]
min = 1
max = 5

[map]
mode = "standard"  # standard | satellite | none
```

## 系统参数
- `system.log_level`：日志等级（debug/info/warn/error）。
- `webengine.remote_debug`：是否启用 WebEngine 远程调试（布署时谨慎开启）。

## 部署建议
- 将 `config.toml` 放在可写目录并与二进制一同部署。  
- 在集中化管理场景下，可把 `config.toml` 做成环境特定模板并使用 CI 填充。