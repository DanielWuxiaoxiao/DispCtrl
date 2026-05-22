##############################################################################
# DispCtrl Docker 编译环境
# 基于 Ubuntu 22.04 — 编译产物可在 Ubuntu 22.04+ (glibc ≥2.35) 上运行
#
# 用法 (由 docker_build.sh 自动调用，也可手动):
#   docker build -f docker/Dockerfile -t dispctrl-builder .
#   docker run --rm -v $(pwd)/deploy:/output dispctrl-builder
#
# 如需目标 Ubuntu 20.04 (glibc 2.31):
#   将 FROM 改为 ubuntu:20.04，并添加 Qt 5.15 PPA:
#     add-apt-repository ppa:okirby/qt515
##############################################################################

ARG UBUNTU_VERSION=22.04
FROM ubuntu:${UBUNTU_VERSION}

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
    # Qt 5 运行时库 (确保齐全)
    libqt5network5 libqt5widgets5 libqt5gui5 \
    libqt5webenginewidgets5 libqt5webenginecore5 \
    libqt5webchannel5 libqt5positioning5 \
    libqt5svg5 libqt5opengl5-dev \
    libqt5quick5 libqt5qml5 libqt5quickwidgets5 \
    # 系统依赖 (X11/GL, WebEngine 运行时)
    libgl1-mesa-dev libxkbcommon-dev libfontconfig1-dev \
    libxcb-xinerama0 libxcb-icccm4 libxcb-image0 \
    libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 \
    libxcb-shape0 libxcb-xfixes0 libxcb-sync1 \
    libnss3 libasound2 \
    # RPATH 修补工具
    patchelf \
    # 压缩
    tar gzip \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
WORKDIR /src

# 复制源码 (由 docker_build.sh 通过 -v 挂载，这里用 COPY 做备选)
# 默认通过 volume 挂载源码，不需要 COPY

# 编译 + 打包入口
CMD ["/bin/bash", "-c", "\
    echo '=== Docker Build: DispCtrl ===' && \
    echo \"Qt version: $(qmake -query QT_VERSION)\" && \
    echo \"GCC: $(g++ --version | head -1)\" && \
    echo '' && \
    cd /src && \
    bash scripts/build_linux.sh Release && \
    bash scripts/package_linux.sh Release && \
    echo '' && \
    if [ -d /output ]; then \
        cp -r deploy/DispCtrl-linux-x64.tar.gz /output/ && \
        echo '=== 已复制到 /output/DispCtrl-linux-x64.tar.gz ===' ; \
    fi \
"]
