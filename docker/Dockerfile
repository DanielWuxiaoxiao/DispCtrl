##############################################################################
# DispCtrl Docker 编译环境
# 基于 Ubuntu 22.04 — 编译产物可在 Ubuntu 22.04+ (glibc ≥2.35) 上运行
#
# Use docker/build_ubuntu.ps1 or docker/docker_build.sh to mount read-only
# source at /input and target-specific output at /output.
#
# 如需目标 Ubuntu 20.04 (glibc 2.31):
#   将 FROM 改为 ubuntu:20.04，并添加 Qt 5.15 PPA:
#     add-apt-repository ppa:okirby/qt515
##############################################################################

ARG UBUNTU_VERSION=22.04
FROM ubuntu:${UBUNTU_VERSION}
ARG UBUNTU_VERSION

# 避免 apt 交互式提示
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# 安装编译工具链 + Qt 5.15 (含 WebEngine) + 打包工具
RUN apt-get update && apt-get install -y --no-install-recommends \
    # 编译工具链
    build-essential cmake ninja-build pkg-config \
    # Qt 5.15 核心开发包
    qtbase5-dev qt5-qmake \
    # Qt WebEngine (含 Chromium)
    qtwebengine5-dev \
    # Qt WebChannel (Qt↔JS 通信)
    libqt5webchannel5-dev \
    libqt5charts5-dev qtconnectivity5-dev libqt5serialport5-dev \
    qt3d5-dev qtdeclarative5-dev \
    # Qt 5 运行时库 (确保齐全)
    libqt5network5 libqt5widgets5 libqt5gui5 \
    libqt5webenginewidgets5 libqt5webenginecore5 \
    libqt5webchannel5 libqt5positioning5 \
    libqt5svg5 libqt5opengl5-dev \
    libqt5quick5 libqt5qml5 libqt5quickwidgets5 \
    libqt5charts5 libqt5bluetooth5 libqt5serialport5 \
    libqt53dcore5 libqt53drender5 libqt53dextras5 libqt53dinput5 libqt53dlogic5 \
    # 系统依赖 (X11/GL, WebEngine 运行时)
    libgl1-mesa-dev libxkbcommon-dev libfontconfig1-dev \
    libxcb-xinerama0 libxcb-icccm4 libxcb-image0 \
    libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 \
    libxcb-shape0 libxcb-xfixes0 libxcb-sync1 \
    libnss3 $(if [ "$UBUNTU_VERSION" = 24.04 ]; then echo libasound2t64; else echo libasound2; fi) \
    libxcb-cursor0 libxkbcommon-x11-0 fonts-noto-cjk ca-certificates \
    # RPATH 修补工具
    patchelf \
    # 压缩
    tar gzip \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
WORKDIR /src
COPY docker/container_build.sh /usr/local/bin/dispctrl-build
RUN sed -i 's/\r$//' /usr/local/bin/dispctrl-build
ENTRYPOINT ["/bin/bash", "/usr/local/bin/dispctrl-build"]
