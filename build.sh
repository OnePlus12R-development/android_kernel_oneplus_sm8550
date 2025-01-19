#! /bin/bash
#
# Kernel compile script for OnePlus 12R
# Copyright (C) 2023-2025 InFeRnO.

# Setup environment
red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'
clear='\033[0m'
KERNEL_PATH=$PWD
ARCH=arm64
DEFCONFIG='gki_defconfig vendor/kalama_GKI.config vendor/oplus/kalama_GKI.config vendor/debugfs.config vendor/oplus/aston.config'
CLANG_PATH=$KERNEL_PATH/.clang
export PATH=$CLANG_PATH/bin:$PATH
KernelSU=false
BUILD_CC="LLVM=1 LLVM_IAS=1"

clone_tools() {
    cd $KERNEL_PATH
    git clone https://git.inferno0230.in/inferno0230/clang-r530567 --depth=1 $KERNEL_PATH/.clang
    git clone https://github.com/Kernel-SU/AnyKernel3 --depth=1 $KERNEL_PATH/AnyKernel3
}

setup_ksu() {
    cd $KERNEL_PATH
    curl -LSs "https://raw.githubusercontent.com/tiann/KernelSU/main/kernel/setup.sh" | bash -
}

ArchLinux() {
    # Check if yay is installed
    if ! command -v yay &> /dev/null
    then
        echo -e "${red}yay is not installed, please install it first!${clear}"
        exit
    else
        yay -S lineageos-devel aosp-devel zstd tar wget curl base-devel lib32-ncurses lib32-zlib lib32-readline --noconfirm
    fi
}

Fedora() {
    curl -LSs "https://raw.githubusercontent.com/akhilnarang/scripts/master/setup/fedora.sh" | bash -
}

Ubuntu() {
    sudo apt install automake bc bison build-essential ccache device-tree-compiler dwarves flex libncurses5-dev make python3 python-is-python3 zlib1g-dev -y
}

build_kernel() {
    cd $KERNEL_PATH
    sed -i 's/ -dirty//g' scripts/setlocalversion
    make O=out ARCH=arm64 CC=clang $BUILD_CC $DEFCONFIG savedefconfig
    # Begin compilation
    start=$(date +%s)
    make O=out CC='ccache clang' CXX='ccache clang++' LTO=thin ARCH=arm64 -j`nproc` ${BUILD_CC} 2>&1 | tee error.log
    if [ -f $KERNEL_PATH/out/arch/arm64/boot/Image ]; then
        echo -e "${green}Kernel Compilation successful!, Zipping...${clear}"
        make_anykernel3_zip
    else
        echo -e "${red}Compilation failed!${clear}"
        echo -e "${red}Check error.log for more info!${clear}"
        exit
    fi
}

make_anykernel3_zip() {
    cd $KERNEL_PATH
    # Extract the kernel version from the Makefile
    zip_name="OP12R-v5.15.$(grep "^SUBLEVEL =" Makefile | awk '{print $3}')-$(date +"%Y%m%d-%H%M").zip"
    cd $KERNEL_PATH/AnyKernel3
    cp $KERNEL_PATH/out/arch/arm64/boot/Image $KERNEL_PATH/AnyKernel3
    zip -r kernel.zip ./*
    mv kernel.zip $KERNEL_PATH/out
    ln -sf $KERNEL_PATH/out/kernel.zip $KERNEL_PATH/out/${zip_name}
    echo -e "${green}out: ${KERNEL_PATH}/out/${zip_name}${clear}"
    echo -e "${clear}"
    echo -e "${green}Completed in $(($(date +%s) - start)) seconds.${clear}"
    cd $KERNEL_PATH
    git reset --hard scripts/setlocalversion
    exit
}

distro_check(){ 
    if [ -f /etc/arch-release ]; then
    echo -e "${green}Arch Linux detected!${clear}"
    ArchLinux
elif [ -f /etc/fedora-release ]; then
    echo -e "${green}Fedora detected!${clear}"
    Fedora
elif [ -f /etc/lsb-release ]; then
    echo -e "${green}Debian based distro detected!${clear}"
    Ubuntu
else
    echo -e "${red}Unsupported OS or ARCH!${clear}"
    exit
fi
}

if [ -d $CLANG_PATH ]; then
    echo -e "${blue}Clang exists, Skipping !${clear}"
else
    echo -e "${red}Clang is missing, Cloning...${clear}"
    echo -e "${yellow}This will take a while...${clear}"
    clone_tools
    distro_check
fi

# Parse Args
while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean) # To clean build kernel
            CLEAN_BUILD=true
            shift
            ;;
        --ksu) # To Enable KernelSU Compilation
            KernelSU=true
            shift
            ;;
        *) # ¯_(ツ)_/¯
            echo "$1: ¯_(ツ)_/¯"
            exit 1
            ;;
    esac
done

if [ "$CLEAN_BUILD" = true ]; then
    rm -rf out/
fi
    

# Check if KernelSU is enabled
if [ "$KernelSU" = true ]; then
    echo -e "${red}KernelSU compilation is Enabled!${clear}"
    setup_ksu
    build_kernel
    make_anykernel3_zip
else
    echo -e "${green}KernelSU compilation is disabled!${clear}"
    build_kernel
    make_anykernel3_zip
fi
