FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build pkg-config \
    qtbase5-dev qt5-qmake \
    libgl1-mesa-dev libxkbcommon-dev libfontconfig1-dev \
    libxcb-xinerama0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 \
    libxcb-randr0 libxcb-render-util0 libxcb-shape0 libxcb-xfixes0 \
    libxcb-sync1 patchelf tar gzip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

CMD ["/bin/bash", "-c", "cd /src && bash scripts/build_linux.sh Release && bash scripts/package_linux.sh Release && if [ -d /output ]; then cp deploy/DispCtrl-linux-x64.tar.gz /output/; fi"]
