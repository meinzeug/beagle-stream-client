BUILD_CONFIG="release"

fail()
{
	echo "$1" 1>&2
	exit 1
}

BUILD_ROOT=$PWD/build
SOURCE_ROOT=$PWD
BUILD_FOLDER=$BUILD_ROOT/build-$BUILD_CONFIG
DEPLOY_FOLDER=$BUILD_ROOT/deploy-$BUILD_CONFIG
INSTALLER_FOLDER=$BUILD_ROOT/installer-$BUILD_CONFIG

LINUXDEPLOY=linuxdeploy-$(uname -m).AppImage

if [ -n "$CI_VERSION" ]; then
  VERSION=$CI_VERSION
else
  VERSION=`cat $SOURCE_ROOT/app/version.txt`
fi

command -v qmake6 >/dev/null 2>&1 || fail "Unable to find 'qmake6' in your PATH!"
command -v $LINUXDEPLOY >/dev/null 2>&1 || fail "Unable to find '$LINUXDEPLOY' in your PATH!"

echo Cleaning output directories
rm -rf $BUILD_FOLDER
rm -rf $DEPLOY_FOLDER
rm -rf $INSTALLER_FOLDER
mkdir $BUILD_ROOT
mkdir $BUILD_FOLDER
mkdir $DEPLOY_FOLDER
mkdir $INSTALLER_FOLDER

echo Configuring the project
pushd $BUILD_FOLDER
# Building with Wayland support will cause linuxdeploy to include libwayland-client.so in the AppImage.
# Since we always use the host implementation of EGL, this can cause libEGL_mesa.so to fail to load due
# to missing symbols from the host's version of libwayland-client.so that aren't present in the older
# version of libwayland-client.so from our AppImage build environment. When this happens, EGL fails to
# work even in X11. To avoid this, we will disable Wayland support for the AppImage.
#
# We disable DRM support because linuxdeploy doesn't bundle the appropriate libraries for Qt EGLFS.
qmake6 $SOURCE_ROOT/moonlight-qt.pro CONFIG+=disable-wayland CONFIG+=disable-libdrm PREFIX=$DEPLOY_FOLDER/usr DEFINES+=APP_IMAGE || fail "Qmake failed!"
popd

echo Compiling Moonlight in $BUILD_CONFIG configuration
pushd $BUILD_FOLDER
make -j$(nproc) $(echo "$BUILD_CONFIG" | tr '[:upper:]' '[:lower:]') || fail "Make failed!"
popd

echo Deploying to staging directory
pushd $BUILD_FOLDER
make install || fail "Make install failed!"
popd

export QML_SOURCES_PATHS=$SOURCE_ROOT/app/gui
export QMAKE=qmake6

echo Creating AppImage
pushd $INSTALLER_FOLDER
# Explicitly bundle FFmpeg libraries built from source so the AppImage is
# self-contained and does not depend on the host libavcodec version.
VERSION=$VERSION $LINUXDEPLOY --appdir $DEPLOY_FOLDER \
  --library=/usr/local/lib/libSDL3.so.0 \
  --library=/usr/local/lib/libSDL2-2.0.so.0 \
  --library=/usr/local/lib/libSDL2_ttf-2.0.so.0 \
  --library=/usr/local/lib/libavcodec.so \
  --library=/usr/local/lib/libavformat.so \
  --library=/usr/local/lib/libswscale.so \
  --library=/usr/local/lib/libavutil.so \
  --plugin qt --output appimage || fail "linuxdeploy failed!"
popd

echo Normalizing BeagleStream AppImage name
APPIMAGE_PATH=$(find "$INSTALLER_FOLDER" -maxdepth 1 -type f -name '*.AppImage' | head -n 1)
if [ -z "$APPIMAGE_PATH" ]; then
  fail "linuxdeployqt did not produce an AppImage"
fi
TARGET_APPIMAGE="$INSTALLER_FOLDER/BeagleStream-$VERSION-x86_64.AppImage"
if [ "$APPIMAGE_PATH" != "$TARGET_APPIMAGE" ]; then
  cp "$APPIMAGE_PATH" "$TARGET_APPIMAGE" || fail "AppImage rename failed!"
fi
chmod +x "$TARGET_APPIMAGE" || fail "AppImage chmod failed!"

echo Build successful
