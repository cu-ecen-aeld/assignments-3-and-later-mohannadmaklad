#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
CURRDIR=$(pwd)
KERNEL_REPO=https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux" ]; then
    #Clone only if the repository does not exist.
    echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
    git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

if [ ! -e ${OUTDIR}/linux/arch/${ARCH}/boot/Image ]; then
    cd "${OUTDIR}/linux"
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}
    sudo make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules_install
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir rootfs
mkdir rootfs/bin rootfs/dev rootfs/etc rootfs/lib rootfs/proc rootfs/sbin rootfs/home rootfs/tmp rootfs/usr rootfs/var
mkdir rootfs/usr/bin rootfs/usr/lib rootfs/usr/sbin
mkdir rootfs/var/lib rootfs/var/lock rootfs/var/log rootfs/var/run rootfs/var/tmp

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    cd ..
fi    

cd busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} distclean 
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install CONFIG_PREFIX="${OUTDIR}/rootfs"
 
 
echo "Busybox dependencies..."
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

echo "Copying sysroot dependencies..."
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp -a "${SYSROOT}"/lib/* ${OUTDIR}/rootfs/lib/
cp -a "${SYSROOT}"/lib64 ${OUTDIR}/rootfs/.

echo "Creating device nodes..."
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1

cd "${CURRDIR}"
make CROSS_COMPILE=${CROSS_COMPILE}

mkdir -p ${OUTDIR}/rootfs/home/conf
cp ./writer ${OUTDIR}/rootfs/home/
cp ./autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp ./finder.sh ${OUTDIR}/rootfs/home/
cp ./finder-test.sh ${OUTDIR}/rootfs/home/
cp -r ./conf/* ${OUTDIR}/rootfs/home/conf

echo "Chowning the root file system..."
sudo chown -R root:root ${OUTDIR}/rootfs

echo "Creating initramfs.cpio.gz.."
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio

