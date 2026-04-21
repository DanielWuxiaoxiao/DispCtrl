#!/bin/bash
##############################################################################
# DispCtrl Linux 桌面快捷方式安装脚本
#
# 用法: 在解压后的部署目录中运行:
#   cd DispCtrl-linux-x64
#   bash install_desktop.sh
#
# 功能:
#   - 创建 .desktop 文件到 ~/.local/share/applications/
#   - 程序出现在 Ubuntu 应用菜单 / GNOME 搜索中
#   - 双击桌面快捷方式即可启动
#
# 卸载:
#   bash install_desktop.sh --uninstall
##############################################################################
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_NAME="DispCtrl"
APP_COMMENT="雷达显示控制系统 (X576)"
APP_CATEGORIES="Science;Engineering;Qt;"
DESKTOP_FILE="dispctrl.desktop"
DESKTOP_DIR="${HOME}/.local/share/applications"

# ---- 卸载模式 ----
if [ "$1" = "--uninstall" ] || [ "$1" = "-u" ]; then
    echo "卸载 DispCtrl 桌面快捷方式..."
    rm -f "${DESKTOP_DIR}/${DESKTOP_FILE}"
    rm -f "${HOME}/Desktop/${DESKTOP_FILE}"
    # 刷新桌面数据库
    if command -v update-desktop-database &>/dev/null; then
        update-desktop-database "${DESKTOP_DIR}" 2>/dev/null || true
    fi
    echo "✓ 已卸载"
    exit 0
fi

# ---- 检查 run.sh 存在 ----
RUN_SCRIPT="${SCRIPT_DIR}/run.sh"
if [ ! -f "$RUN_SCRIPT" ]; then
    echo "错误: 未找到 run.sh"
    echo "请在 DispCtrl-linux-x64 部署目录中运行此脚本"
    exit 1
fi

# ---- 查找图标 ----
ICON_PATH=""
for candidate in \
    "${SCRIPT_DIR}/bin/resources/icon/app.png" \
    "${SCRIPT_DIR}/bin/resources/icon/radar.png" \
    "${SCRIPT_DIR}/bin/resources/icon/logo.png" \
    "${SCRIPT_DIR}/bin/resources/icon/DispCtrl.png"; do
    if [ -f "$candidate" ]; then
        ICON_PATH="$candidate"
        break
    fi
done

# 如果没找到图标，使用系统默认
if [ -z "$ICON_PATH" ]; then
    # 查找 resources/icon/ 下任意 png
    FIRST_PNG=$(find "${SCRIPT_DIR}/bin/resources/icon/" -name "*.png" -type f 2>/dev/null | head -1)
    if [ -n "$FIRST_PNG" ]; then
        ICON_PATH="$FIRST_PNG"
    else
        ICON_PATH="utilities-system-monitor"  # 系统默认图标
    fi
fi

echo "============================================"
echo "  DispCtrl 桌面快捷方式安装"
echo "============================================"
echo "  安装目录: ${SCRIPT_DIR}"
echo "  启动脚本: ${RUN_SCRIPT}"
echo "  图标:     ${ICON_PATH}"
echo ""

# ---- 创建 .desktop 文件 ----
mkdir -p "${DESKTOP_DIR}"

cat > "${DESKTOP_DIR}/${DESKTOP_FILE}" << DESKTOP_EOF
[Desktop Entry]
Type=Application
Name=${APP_NAME}
Comment=${APP_COMMENT}
Exec=${RUN_SCRIPT}
Icon=${ICON_PATH}
Terminal=false
Categories=${APP_CATEGORIES}
StartupNotify=true
StartupWMClass=DispCtrl
DESKTOP_EOF

echo "✓ 已创建: ${DESKTOP_DIR}/${DESKTOP_FILE}"

# ---- 也复制到桌面（如果存在） ----
DESKTOP_PATH="${HOME}/Desktop"
if [ ! -d "$DESKTOP_PATH" ]; then
    # 中文 Ubuntu 桌面目录可能是"桌面"
    DESKTOP_PATH="${HOME}/桌面"
fi

if [ -d "$DESKTOP_PATH" ]; then
    cp "${DESKTOP_DIR}/${DESKTOP_FILE}" "${DESKTOP_PATH}/"
    chmod +x "${DESKTOP_PATH}/${DESKTOP_FILE}"
    # GNOME 需要标记为可信任（Ubuntu 20.04+）
    if command -v gio &>/dev/null; then
        gio set "${DESKTOP_PATH}/${DESKTOP_FILE}" metadata::trusted true 2>/dev/null || true
    fi
    echo "✓ 已复制到桌面: ${DESKTOP_PATH}/${DESKTOP_FILE}"
fi

# ---- 刷新桌面数据库 ----
if command -v update-desktop-database &>/dev/null; then
    update-desktop-database "${DESKTOP_DIR}" 2>/dev/null || true
fi

echo ""
echo "============================================"
echo "  安装完成!"
echo "============================================"
echo "  现在可以通过以下方式启动 DispCtrl:"
echo "    1. 在应用菜单中搜索 'DispCtrl'"
echo "    2. 双击桌面上的 DispCtrl 图标"
echo "    3. 命令行: ${RUN_SCRIPT}"
echo ""
echo "  卸载: bash $0 --uninstall"
echo "============================================"
