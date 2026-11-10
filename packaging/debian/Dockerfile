FROM debian:trixie
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates build-essential cmake ninja-build qt6-base-dev \
    qt6-declarative-dev libqt6svg6 qt6-webengine-dev qt6-webchannel-dev libkf6wallet-dev extra-cmake-modules dpkg-dev file \
    pkg-config libpipewire-0.3-dev \
    qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    qml6-module-qtquick-window qml6-module-qtqml-workerscript qml6-module-qttest \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPHOSRACK_SPOTIFY_CLIENT_ID= \
    && cmake --build /build --parallel 2 \
    && ctest --test-dir /build --output-on-failure \
    && cd /build && cpack -G DEB
