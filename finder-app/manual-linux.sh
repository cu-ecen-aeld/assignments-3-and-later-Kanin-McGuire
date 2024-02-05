# #!/bin/bash
# # Script outline to install and build kernel.
# # Author: Siddhant Jajoo.

# set -e
# set -u

# OUTDIR=/tmp/aeld
# KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
# KERNEL_VERSION=v5.1.10
# BUSYBOX_VERSION=1_33_1
# FINDER_APP_DIR=$(realpath $(dirname $0))
# ARCH=arm64
# CROSS_COMPILE=aarch64-none-linux-gnu-

# if [ $# -lt 1 ]
# then
# 	echo "Using default directory ${OUTDIR} for output"
# else
# 	OUTDIR=$1
# 	echo "Using passed directory ${OUTDIR} for output"
# fi

# mkdir -p ${OUTDIR}

# cd "$OUTDIR"
# if [ ! -d "${OUTDIR}/linux-stable" ]; then
#     #Clone only if the repository does not exist.
# 	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
# 	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
# fi
# if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
#     cd linux-stable
#     echo "Checking out version ${KERNEL_VERSION}"
#     git checkout ${KERNEL_VERSION}

#     # TODO: Add your kernel build steps here
# fi

# echo "Adding the Image in outdir"

# echo "Creating the staging directory for the root filesystem"
# cd "$OUTDIR"
# if [ -d "${OUTDIR}/rootfs" ]
# then
# 	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
#     sudo rm  -rf ${OUTDIR}/rootfs
# fi

# # TODO: Create necessary base directories

# cd "$OUTDIR"
# if [ ! -d "${OUTDIR}/busybox" ]
# then
# git clone git://busybox.net/busybox.git
#     cd busybox
#     git checkout ${BUSYBOX_VERSION}
#     # TODO:  Configure busybox
# else
#     cd busybox
# fi

# # TODO: Make and install busybox

# echo "Library dependencies"
# ${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
# ${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# # TODO: Add library dependencies to rootfs

# # TODO: Make device nodes

# # TODO: Clean and build the writer utility

# # TODO: Copy the finder related scripts and executables to the /home directory
# # on the target rootfs

# # TODO: Chown the root directory

# # TODO: Create initramfs.cpio.gz
#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
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

# mkdir -p ${OUTDIR}
if mkdir -p "$OUTDIR"; then
    echo "Directory created successfully."
else
    echo "Failed to create the directory."
    exit 1  # Exit with an error code
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    sudo apt-get update
    sudo apt-get install -y --no-install-recommends bc u-boot-tools kmod cpio flex bison libssl-dev psmisc
    sudo apt-get install -y qemu-system-arm

    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"

scp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi


# TODO: Create necessary base directories
# cd "/rootfs"
# initramfs

mkdir -p rootfs/bin rootfs/dev rootfs/etc rootfs/home rootfs/lib rootfs/lib64 rootfs/proc rootfs/sbin rootfs/sys rootfs/tmp rootfs/usr rootfs/var
mkdir -p rootfs/usr/bin rootfs/usr/lib rootfs/use/sbin
mkdir -p rootfs/var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi


# TODO: Make and install busybox
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/busybox ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

cd ../rootfs

# TODO: Add library dependencies to rootfs
library_path=$(sudo find / -name ld-linux-aarch64.so.1 2>/dev/null || true)
library_path1=$(sudo find / -name libm.so.6 2>/dev/null || true)
library_path2=$(sudo find / -name libresolv.so.2 2>/dev/null || true)
library_path3=$(sudo find / -name libc.so.6 2>/dev/null || true)

# Create symbolic links for the libraries
sudo ln -sf  ${library_path1} lib64
sudo ln -sf  ${library_path2} lib64
sudo ln -sf  ${library_path3} lib64

# Create symbolic link for the dynamic linker
sudo ln -sf  ${library_path} lib

# ls ../rootfs/lib
# ls ../rootfs/lib64

# echo "Library dependencies"
# ${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
# ${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"


cd ${OUTDIR}/rootfs
# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1 


cd $FINDER_APP_DIR

# TODO: Clean and build the writer utility
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
scp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
scp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
scp ${FINDER_APP_DIR}/writer.c ${OUTDIR}/rootfs/home
scp -r ${FINDER_APP_DIR}/conf ${OUTDIR}/rootfs/home
# scp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home
scp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
scp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home


# cd "$OUTDIR/r"
# ls $OUTDIR/rootfs/home

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
# cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
echo "$(pwd)"
gzip -f ${OUTDIR}/initramfs.cpio