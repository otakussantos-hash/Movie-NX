
set -e

BUILD_DIR=cmake-build-switch


cd "$(dirname $0)/.."
git config --global --add safe.directory `pwd`

# Aggiorna i pacchetti nel container DevkitPro prima della build
dkp-pacman -Syu --noconfirm

BASE_URL="https://github.com/xfangfang/wiliwili/releases/download/v0.1.0/"

PKGS=(
    "switch-libass-0.17.1-1-any.pkg.tar.zst"
    "switch-ffmpeg-6.1-5-any.pkg.tar.zst"
    "switch-libmpv-0.36.0-2-any.pkg.tar.zst"
    "switch-nspmini-48d4fc2-1-any.pkg.tar.xz"
    "hacBrewPack-3.05-1-any.pkg.tar.zst"
)
for PKG in "${PKGS[@]}"; do
    [ -f "${PKG}" ] || curl -LO ${BASE_URL}${PKG}
    dkp-pacman -U --noconfirm ${PKG}
done


# GITHUB_TOKEN is optional but pass it if available
GITHUB_TOKEN_FLAG=""
if [ -n "${GITHUB_TOKEN}" ]; then
    GITHUB_TOKEN_FLAG="-DGITHUB_TOKEN=\"${GITHUB_TOKEN}\""
fi

# Disable unity build by default for stability on Switch
# Can be re-enabled with ENABLE_UNITY_BUILD=true environment variable
UNITY_BUILD_FLAG="-DBRLS_UNITY_BUILD=OFF"
if [ "${ENABLE_UNITY_BUILD}" = "true" ]; then
    UNITY_BUILD_FLAG="-DBRLS_UNITY_BUILD=ON"
fi

EXTRA_FLAGS=()
if [ -n "${GA_ID}" ] && [ -n "${GA_KEY}" ]; then
    EXTRA_FLAGS+=(-DANALYTICS=ON "-DANALYTICS_ID=${GA_ID}" "-DANALYTICS_KEY=${GA_KEY}")
fi
[ -n "${SERVER_URL}" ] && EXTRA_FLAGS+=("-DSERVER_URL=${SERVER_URL}")
[ -n "${SERVER_TOKEN}" ] && EXTRA_FLAGS+=("-DSERVER_TOKEN=${SERVER_TOKEN}")
[ -n "${M3U8_URL}" ] && EXTRA_FLAGS+=("-DM3U8_URL=${M3U8_URL}")

cmake -B ${BUILD_DIR} \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILTIN_NSP=ON \
  -DPLATFORM_SWITCH=ON \
  ${UNITY_BUILD_FLAG} \
  -DCMAKE_UNITY_BUILD_BATCH_SIZE=16 \
  ${EXTRA_FLAGS[@]} \
  ${GITHUB_TOKEN_FLAG}

make -C ${BUILD_DIR} TsVitch.nro -j$(nproc) || {
    echo "=== Parallel build failed, retrying serially to show the real error ==="
    make -C ${BUILD_DIR} TsVitch.nro -j1 VERBOSE=1 2>&1 | tail -n 80
    exit 1
}
