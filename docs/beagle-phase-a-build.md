# BeagleStream Client Phase A Build

This fork keeps upstream Moonlight-Qt behavior when `/etc/beagle/enrollment.conf`
is missing. Beagle allocation, token-as-PIN pairing and WireGuard peer activation
are only used when enrollment config is present and valid.

## Ubuntu 24.04 build host

Install the build prerequisites used for the Phase A verification build:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential git qmake6-bin qt5-qmake qtbase5-dev qtbase5-dev-tools \
  qtdeclarative5-dev qtquickcontrols2-5-dev qml-module-qtquick2 \
  qml-module-qtquick-controls2 qml-module-qtquick-layouts \
  qml-module-qtquick-window2 qml-module-qtgraphicaleffects \
  libqt5svg5-dev libsdl2-dev libsdl2-ttf-dev libssl-dev libopus-dev \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavfilter-dev \
  libva-dev libvdpau-dev libdrm-dev libx11-dev libxrandr-dev libxss-dev \
  libegl1-mesa-dev libgl1-mesa-dev libwayland-dev wayland-protocols imagemagick
```

Initialize upstream submodules:

```bash
git submodule update --init --recursive
```

## Beagle build

```bash
mkdir -p build/beagle-check
cd build/beagle-check
qmake ../../moonlight-qt.pro
make -j"$(nproc)"
```

Runtime enrollment is read from `/etc/beagle/enrollment.conf`. The enrollment
token is sent only as the `X-Beagle-Token` HTTP header and must not be committed.

## Release AppImage

The `BeagleStream Client Release` workflow runs on `beagle/phase-a` pushes and
publishes a mutable prerelease named `beagle-phase-a`.

Stable Thin-Client build URL:

```text
https://github.com/meinzeug/beagle-stream-client/releases/download/beagle-phase-a/BeagleStream-latest-x86_64.AppImage
```

Use it in Beagle OS artifact builds via:

```bash
PVE_THIN_CLIENT_BEAGLE_STREAM_CLIENT_URL="https://github.com/meinzeug/beagle-stream-client/releases/download/beagle-phase-a/BeagleStream-latest-x86_64.AppImage"
```
