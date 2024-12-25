#!/usr/bin/env bash

set -eo pipefail

cd $(dirname $0)
CWD=$(pwd)
TP_DIR=${CWD}

PARALLEL=$(getconf _NPROCESSORS_ONLN)
PACKAGE=""

function usage() {
    echo "Usage:"
    echo "    $0 [OPTIONS]..."
    echo ""
    echo "Options:"
    echo "    --j [parallel]"
    echo "        Specify the number of parallelism."
    echo "    --package [PACKAGE]"
    echo "        Specify the package, build all packages of is empty."
    echo "    --help"
    echo "        Show help message and exit."
    exit 1
}

while test $# -gt 0; do
    case $1 in
    --j)
        PARALLEL=$2
        shift 2
        ;;
    --package)
        PACKAGE=$2
        shift 2
        ;;
    --help)
        usage
        ;;
    *)
        echo Invalid parameters \"$@\".
        usage
        ;;
    esac
done

echo PARALLEL=$PARALLEL
echo PACKAGE=$PACKAGE

mkdir -p "${TP_DIR}/src"
mkdir -p "${TP_DIR}/installed/lib64"
pushd "${TP_DIR}/installed"/
ln -sf lib64 lib
popd

TP_SOURCE_DIR="${TP_DIR}/src"
TP_INSTALL_DIR="${TP_DIR}/installed"
TP_INCLUDE_DIR="${TP_INSTALL_DIR}/include"
TP_LIB_DIR="${TP_INSTALL_DIR}/lib"
TP_PATCH_DIR="${TP_DIR}/patches"


echo "SOURCE_DIR: ${TP_SOURCE_DIR}"
echo "INSTALL_DIR: ${TP_INSTALL_DIR}"
echo "INCLUDE_DIR: ${TP_INCLUDE_DIR}"
echo "LID_DIR: ${TP_LIB_DIR}"
echo "PATCH_DIR: ${TP_PATCH_DIR}"

# TODO: 检查编译工具和相关版本

function check_md5() {
    local FILE=$1
    local EXPECT=$2

    md5="$(md5sum "${FILE}")"
    if [[ "${md5}" != "${MD5SUM}  ${FILE}" ]]; then
        echo "${FILE} md5sum check failed!"
        echo -e "except-md5 ${EXPECT} \nactual-md5 ${md5}"
        exit 1
    fi
}

function build_openblas() {
    local URL="https://gh.llkk.cc/https://github.com/OpenMathLib/OpenBLAS/archive/refs/tags/v0.3.28.tar.gz"
    local FILE=OpenBLAS-0.3.2.tar.gz
    local DIR=OpenBLAS-0.3.28
    local MD5SUM="0f54185b6ef804173c01b9a40520a0e8"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    rm -rf build
    mkdir -p build
    make -j ${PARALLEL}
    make PREFIX=./build install

    mkdir -p ${TP_INCLUDE_DIR}/blas
    cp -r ./build/include/* ${TP_INCLUDE_DIR}/blas
    cp -r ./build/lib/*.a ${TP_LIB_DIR}/
}

function build_faiss() {
    build_openblas

    local URL="https://gh.llkk.cc/https://github.com/facebookresearch/faiss/archive/refs/tags/v1.9.0.tar.gz"
    local FILE=faiss-1.9.0.tar.gz
    local DIR=faiss-1.9.0
    local MD5SUM="db62643ba325b296eeb84dc73897fe81"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    PATCHED_MARK="patched_mark"
    if [[ ! -f "${PATCHED_MARK}" ]]; then
        patch -p1 <"${TP_PATCH_DIR}/faiss-1.9.0.patch"
        touch "${PATCHED_MARK}"
    fi
    cmake -B build .  -DCMAKE_BUILD_TYPE=Release \
                    -DFAISS_ENABLE_GPU=OFF \
                    -DFAISS_ENABLE_PYTHON=OFF \
                    -DBUILD_TESTING=OFF \
                    -DBUILD_SHARED_LIBS=OFF \
                    -DCMAKE_INSTALL_PREFIX=$TP_INSTALL_DIR \
                    -DTP_INSTALL_DIR=$TP_INSTALL_DIR
    make -C build -j ${PARALLEL} faiss install
}

function build_hnswlib() {
    local URL="https://gh.llkk.cc/https://github.com/nmslib/hnswlib/archive/refs/tags/v0.8.0.tar.gz"
    local FILE=hnswlib-0.8.0.tar.gz
    local DIR=hnswlib-0.8.0
    local MD5SUM="126c5c6b7d8e71c6e7c70dc4d5f3933e"

     [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    cp -r hnswlib ${TP_INCLUDE_DIR}/hnswlib

}

function build_rapidjson() {
    local REPO_URL="https://gh.llkk.cc/https://github.com/Tencent/rapidjson.git"
    local DIR=rapidjson

    # 检查是否已经 clone 仓库，如果没有则执行 clone
    if [ ! -d "${TP_SOURCE_DIR}/${DIR}" ]; then
        git clone ${REPO_URL} ${TP_SOURCE_DIR}/${DIR}
    fi

    cd ${TP_SOURCE_DIR}/${DIR}

    # 如果已经 clone，确保拉取最新的代码
    git fetch --all
    git pull origin $(git rev-parse --abbrev-ref HEAD)

    # 配置和安装
    cmake -B build . -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" \
                    -DRAPIDJSON_BUILD_DOC=OFF \
                    -DRAPIDJSON_BUILD_EXAMPLES=OFF \
                    -DRAPIDJSON_BUILD_TESTS=OFF
    make -C build install
}

function build_gtest() {
    local URL="https://gh.llkk.cc/https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz"
    local FILE=googletest-1.15.2.tar.gz
    local DIR=googletest-1.15.2
    local MD5SUM="7e11f6cfcf6498324ac82d567dcb891e"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    cmake -B build . -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" 
    make -C build install
}

function build_backward() {
    local URL="https://gh.llkk.cc/https://github.com/bombela/backward-cpp/archive/refs/tags/v1.6.tar.gz"
    local FILE=v1.6.tar.gz
    local DIR=backward-cpp-1.6
    local MD5SUM="0facf6e0fb35ed0f3cd069424a1dc79a"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    cmake -B build . -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" 
    make -C build install
}

function build_httplib() {
    local URL="https://gh.llkk.cc/https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.1.tar.gz"
    local FILE=cpp-httplib-0.18.1.tar.gz
    local DIR=cpp-httplib-0.18.1
    local MD5SUM="a2427747a7c352fee8a1cc9e4db87168"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    mkdir -p ${TP_INCLUDE_DIR}/httplib
    cp ./httplib.h ${TP_INCLUDE_DIR}/httplib/
}

function build_spdlog() {
    local URL="https://gh.llkk.cc/https://github.com/gabime/spdlog/archive/refs/tags/v1.14.1.tar.gz"
    local FILE=spdlog-1.14.1.tar.gz
    local DIR=spdlog-1.14.1
    local MD5SUM="f2c3f15c20e67b261836ff7bfda302cf"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    cmake -B build . -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}"
    make -C build -j ${PARALLEL} install
}

function build_gflags() {
    local URL="https://gh.llkk.cc/https://github.com/gflags/gflags/archive/refs/tags/v2.2.1.tar.gz"
    local FILE=gflags-2.2.1.tar.gz
    local DIR=gflags-2.2.1
    local MD5SUM="b98e772b4490c84fc5a87681973f75d1"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    cmake -B build . -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" \
                    -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=On
    make -C build -j ${PARALLEL} install
}

function build_glog() {
    local URL="https://gh.llkk.cc/https://github.com/google/glog/archive/refs/tags/v0.6.0.tar.gz"
    local FILE=glog-0.6.0.tar.gz
    local DIR=glog-0.6.0
    local MD5SUM="c98a6068bc9b8ad9cebaca625ca73aa2"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    cmake -S . -B build -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" \
                                            -DCMAKE_BUILD_TYPE=Release \
                                            -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                                            -DWITH_UNWIND=OFF \
                                            -DBUILD_SHARED_LIBS=OFF \
                                            -DWITH_TLS=OFF
    cmake --build build -j ${PARALLEL} --target install
}

function build_zlib() {
    local URL="https://gh.llkk.cc/https://github.com/madler/zlib/archive/refs/tags/v1.2.13.tar.gz"
    local FILE=zlib-1.2.13.tar.gz
    local DIR=zlib-1.2.13
    local MD5SUM="9c7d356c5acaa563555490676ca14d23"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    rm -rf build
    mkdir -p build
    CFLAGS="-O3 -fPIC" \
    CPPFLAGS="-I${TP_INCLUDE_DIR}" \
    LDFLAGS="-L${TP_LIB_DIR}" \
    ./configure --prefix="${TP_SOURCE_DIR}/${DIR}/build"
    make -j ${PARALLEL} install
    cp -r ./build/include/* ${TP_INCLUDE_DIR}/
    cp -r ./build/lib/*.a ${TP_LIB_DIR}/
}

function build_protobuf() {
    local URL="https://gh.llkk.cc/https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.17.3.tar.gz"
    local FILE=protobuf-3.17.3.tar.gz
    local DIR=protobuf-3.17.3
    local MD5SUM="d7f8e0e3ffeac721e18cdf898eff7d31"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    mkdir -p cmake/build
    cd cmake/build
    CXXFLAGS="-I${TP_INCLUDE_DIR}" \
    cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -Dprotobuf_BUILD_SHARED_LIBS=OFF \
        -Dprotobuf_BUILD_TESTS=OFF \
        -DZLIB_LIBRARY="${TP_LIB_DIR}/libz.a" \
        -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" ../
    make -j ${PARALLEL} install
}

function build_leveldb() {
    local URL="https://gh.llkk.cc/https://github.com/google/leveldb/archive/refs/tags/1.23.tar.gz"
    local FILE=leveldb-1.23.tar.gz
    local DIR=leveldb-1.23
    local MD5SUM="afbde776fb8760312009963f09a586c7"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}

    CXXFLAGS="-fPIC" cmake -B build . -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" -DLEVELDB_BUILD_BENCHMARKS=OFF \
        -DLEVELDB_BUILD_TESTS=OFF
    make -C build -j ${PARALLEL} install
}

function build_openssl() {
    local URL="https://gh.llkk.cc/https://github.com/openssl/openssl/archive/refs/tags/OpenSSL_1_1_1.tar.gz"
    local FILE=openssl-OpenSSL_1_1_1.tar.gz
    local DIR=openssl-OpenSSL_1_1_1
    local MD5SUM="d65944e4aa4de6ad9858e02c82d85183"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}

    OPENSSL_PLATFORM="linux-x86_64"
    ./Configure --prefix="${TP_INSTALL_DIR}" --with-rand-seed=devrandom -shared "${OPENSSL_PLATFORM}"
    make -j ${PARALLEL}
    make install_sw
    if [[ -f "${TP_INSTALL_DIR}/lib64/libcrypto.so" ]]; then
        rm -rf "${TP_INSTALL_DIR}"/lib64/libcrypto.so*
    fi
    if [[ -f "${TP_INSTALL_DIR}/lib64/libssl.so" ]]; then
        rm -rf "${TP_INSTALL_DIR}"/lib64/libssl.so*
    fi

}

function build_brpc() {
    local URL="https://gh.llkk.cc/https://github.com/apache/brpc/archive/refs/tags/1.11.0.tar.gz"
    local FILE=brpc-1.11.0.tar.gz
    local DIR=brpc-1.11.0
    local MD5SUM="f55e582fb8032768f9070865b48e892d"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    PATCHED_MARK="patched_mark"
    if [[ ! -f "${PATCHED_MARK}" ]]; then
        patch -p1 <"${TP_PATCH_DIR}/brpc-1.11.0.patch"
        touch "${PATCHED_MARK}"
    fi
    sed '/set(OPENSSL_ROOT_DIR/,/)/ d' ./CMakeLists.txt >./CMakeLists.txt.bak
    mv ./CMakeLists.txt.bak ./CMakeLists.txt
    cmake -B build . -DBUILD_SHARED_LIBS=ON -DWITH_GLOG=ON -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" \
        -DCMAKE_LIBRARY_PATH="${TP_INSTALL_DIR}/lib64" -DCMAKE_INCLUDE_PATH="${TP_INSTALL_DIR}/include" \
        -DBUILD_BRPC_TOOLS=OFF \
        -DWITH_SNAPPY=ON \
        -DPROTOBUF_PROTOC_EXECUTABLE="${TP_INSTALL_DIR}/bin/protoc"
    make -C build -j ${PARALLEL} install
    if [[ -f "${TP_INSTALL_DIR}/lib/libbrpc.so" ]]; then
        rm -rf "${TP_INSTALL_DIR}"/lib/libbrpc.so*
    fi
}
        

function build_snappy() {
    local URL="https://gh.llkk.cc/https://github.com/google/snappy/archive/refs/tags/1.2.1.tar.gz"
    local FILE=snappy-1.2.1.tar.gz
    local DIR=snappy-1.2.1
    local MD5SUM="dd6f9b667e69491e1dbf7419bdf68823"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}

    cmake -B build . -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
            -DCMAKE_INSTALL_INCLUDEDIR="${TP_INCLUDE_DIR}"/snappy \
            -DSNAPPY_BUILD_TESTS=OFF -DSNAPPY_BUILD_BENCHMARKS=OFF

    make -C build -j ${PARALLEL} install
}

function build_lz4() {
    local URL="https://gh.llkk.cc/https://github.com/lz4/lz4/archive/refs/tags/v1.9.4.tar.gz"
    local FILE=lz4-1.9.4.tar.gz
    local DIR=lz4-1.9.4
    local MD5SUM="e9286adb64040071c5e23498bf753261"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    make -j ${PARALLEL} install PREFIX="${TP_INSTALL_DIR}" BUILD_SHARED=no INCLUDEDIR="${TP_INCLUDE_DIR}/lz4"
}

function build_bzip() {
    local URL="https://fossies.org/linux/misc/bzip2-1.0.8.tar.gz"
    local FILE=bzip2-1.0.8.tar.gz
    local DIR=bzip2-1.0.8
    local MD5SUM="67e051268d0c475ea773822f7500d0e5"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    make -j ${PARALLEL} install PREFIX="${TP_INSTALL_DIR}"
}

function build_rocksdb() {
    local URL="https://gh.llkk.cc/https://github.com/facebook/rocksdb/archive/refs/tags/v8.0.0.tar.gz"
    local FILE=rocksdb-8.0.0.tar.gz
    local DIR=rocksdb-8.0.0
    local MD5SUM="148458e1efd16cc235a0ddb2796313f0"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    CFLAGS="-I ${TP_INCLUDE_DIR} -I ${TP_INCLUDE_DIR}/snappy -I ${TP_INCLUDE_DIR}/lz4" \
    LDFLAGS="-static-libstdc++ -static-libgcc" PORTABLE=1 make USE_RTTI=1 -j ${PARALLEL} static_lib
    cp librocksdb.a ${TP_LIB_DIR}/librocksdb.a
    cp -r include/rocksdb ${TP_INCLUDE_DIR}/
}

function build_roaringbitmap() {
    local URL="https://gh.llkk.cc/https://github.com/RoaringBitmap/CRoaring/archive/refs/tags/v2.1.2.tar.gz"
    local FILE=CRoaring-2.1.2.tar.gz
    local DIR=CRoaring-2.1.2
    local MD5SUM="419bfbafdf93e9a7e6cdc234454908fc"

    [ -f ${TP_SOURCE_DIR}/${FILE} ] || wget $URL -O ${TP_SOURCE_DIR}/${FILE}
    check_md5 ${TP_SOURCE_DIR}/${FILE} $MD5SUM
    [ -d ${TP_SOURCE_DIR}/${DIR} ] || tar xvf ${TP_SOURCE_DIR}/${FILE} -C ${TP_SOURCE_DIR}

    cd ${TP_SOURCE_DIR}/${DIR}
    cmake -B build . -DROARING_BUILD_STATIC=ON -DCMAKE_INSTALL_PREFIX="${TP_INSTALL_DIR}" \
          -DENABLE_ROARING_TESTS=OFF

    make -C build -j ${PARALLEL} install
}

PACKAGES=(
    "faiss"
    "hnswlib"
    "rapidjson"
    "httplib"
    "spdlog"
    "gflags"
    "glog"
    "zlib"
    "protobuf"
    "leveldb"
    "openssl"
    "snappy"
    "brpc"
    "lz4"
    "bzip"
    "rocksdb"
    "roaringbitmap"
    "gtest"
    "backward"
)

function build() {
    local package=$1
    if [[ -z "$package" ]]; then
        for pkg in "${PACKAGES[@]}"; do
            build_"$pkg"
        done
    else
        if [[ " ${PACKAGES[*]} " == *" $package "* ]]; then
            build_"$package"
        else
            echo "Package $package not found."
        fi
    fi
    echo "build finish!"
}

build $PACKAGE

# appreciate Xiaoccer , the original build.sh from git@github.com:Xiaoccer/TinyVecDB.git