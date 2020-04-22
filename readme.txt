zlib 编译
git clone https://github.com/madler/zlib.git          V=1.2.8
修改Makefile

ffilib编译
下载 https://sourceware.org/libffi/      V=3.2.1
./configure  CC=/home/taohaiwu/works/project/adas/c211_repo/ti_components/os_tools/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --host=arm-linux --prefix=/home/taohaiwu/works/project/adas/c211_repo/rootfs/common/targetfs/usr
make

make install


glib编译
下载 http://ftp.gnome.org/pub/gnome/sources/glib/2.46/
touch glib.cache

把以下配置放入glib.cache 
glib_cv_long_long_format=ll
glib_cv_stack_grows=no
glib_cv_have_strlcpy=no
glib_cv_have_qsort_r=yes
glib_cv_va_val_copy=yes
glib_cv_uscore=no
glib_cv_rtldglobal_broken=no
ac_cv_func_posix_getpwuid_r=yes
ac_cv_func_posix_getgrgid_r=yes

./configure --prefix=/home/taohaiwu/works/project/adas/c211_repo/rootfs/common/targetfs/usr   CC=/home/taohaiwu/works/project/adas/c211_repo/ti_components/os_tools/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --host=arm-linux LIBFFI_CFLAGS="-I/home/taohaiwu/works/project/adas/c211_repo/rootfs/common/targetfs/usr/lib/libffi-3.2.1/include" LIBFFI_LIBS="-lffi -L/home/taohaiwu/works/project/adas/c211_repo/rootfs/common/targetfs/usr/lib" ZLIB_CFLAGS="-I/home/taohaiwu/works/project/adas/c211_repo/rootfs/common/targetfs/usr/include" ZLIB_LIBS="-lz -L/home/taohaiwu/works/project/adas/c211_repo/rootfs/common/targetfs/usr/lib"   --cache-file=glib.cache   --disable-selinux  --disable-xattr --disable-libelf 
 make 
 make install
 
 
Your branch is behind 'ofi/cag_c211_dev' by 18 commits, and can be fast-forwarded.
  (use "git pull" to update your local branch)
Changes not staged for commit:
  (use "git add <file>..." to update what will be committed)
  (use "git checkout -- <file>..." to discard changes in working directory)

        modified:   Makefile.Inc
        modified:   build/build_plugin_bad.sh
        modified:   build/envsetup.sh

no changes added to commit (use "git add" and/or "git commit -a")
taohaiwu@ubuntu:~/works/project/adas/c211/workspace/dvr$ git diff
diff --git a/Makefile.Inc b/Makefile.Inc
index d25bf31..772c807 100755
--- a/Makefile.Inc
+++ b/Makefile.Inc
@@ -3,7 +3,7 @@
 ##COMPILE tools
 ##
 ##
-PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-
+PREFIX=/home/taohaiwu/works/project/adas/c211/ti_components/os_tools/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
 AS     = $(PREFIX)as
 CC     = $(PREFIX)gcc
 CXX    = $(PREFIX)g++
@@ -37,7 +37,7 @@ CPPFLAGS = $(CPUFLAGS)
 ## 
 CONFIGOPTION = --host=arm-linux-gnueabihf
 
-RELEASEDIR := $(DVR_TOP)/targetfs/
+RELEASEDIR := $(DVR_TOP)/../../rootfs/targetfs
 SPECIFIED_INCLUDE_PATH=${RELEASEDIR}/usr/include
 SPECIFIED_LIB_PATH=${RELEASEDIR}/usr/lib
 PLUGIN_DIR=${SPECIFIED_LIB_PATH}/gstreamer-1.0
diff --git a/build/build_plugin_bad.sh b/build/build_plugin_bad.sh
index 886261a..514c23b 100755
--- a/build/build_plugin_bad.sh
+++ b/build/build_plugin_bad.sh
@@ -38,7 +38,7 @@ export LIBDCE_CFLAGS="-I${SPECIFIED_INCLUDE_PATH}/dce"
 
 MODULE_NAME=gst-plugins-bad-1.2.3
 
-OPTIONS="--disable-orc --disable-wayland --disable-sbc --disable-vdpau --disable-decklink --disable-uvch264 --disable-bluez --disable-eglgles --disable-curl"
+OPTIONS="--disable-orc --disable-dash --disable-smoothstreaming --disable-wayland --disable-sbc --disable-vdpau --disable-decklink --disable-uvch264 --disable-bluez --disable-eglgles --disable-curl"
 
 start_build
 
diff --git a/build/envsetup.sh b/build/envsetup.sh
index 52d35e0..f3dfc03 100755
--- a/build/envsetup.sh
+++ b/build/envsetup.sh
@@ -6,7 +6,7 @@
 ##
 ##
 #PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-
-PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
+PREFIX=/home/taohaiwu/works/project/adas/c211/ti_components/os_tools/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
 AS=${PREFIX}gcc
 CC=${PREFIX}gcc
 CXX=${PREFIX}g++
@@ -46,15 +46,15 @@ export CFLAGS=${CPUFLAGS}
 CURRENT_PATH=`pwd`
 
 VERSION=1.2.3
-SYS_ROOT_DIR=${CURRENT_PATH}/../../rootfs
+SYS_ROOT_DIR=${CURRENT_PATH}/../../../rootfs/targetfs
 HOST=arm-linux-gnueabihf
 INSTALL_PATH=${SYS_ROOT_DIR}/usr
 
-DVR_TOP=${CURRENT_PATH}/../
+DVR_TOP=${CURRENT_PATH}/..
 GSTREAMER_ROOT_SOURCE_PATH=${DVR_TOP}/gstreamer
 SDK_SOURCE_PATH=${DVR_TOP}/dvr_sdk
 SPECIFIED_INCLUDE_PATH=${SYS_ROOT_DIR}/usr/include
 SPECIFIED_LIB_PATH=${SYS_ROOT_DIR}/usr/lib
 
 export PKG_CONFIG_PATH=${SYS_ROOT_DIR}/usr/lib/pkgconfig
-export PATH=$PATH:/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin
+export PATH=$PATH:/home/taohaiwu/works/project/adas/c211/ti_components/os_tools/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/
\ No newline at end of file

//************编译ubuntu gstreamer****************************************************************************************************/
git clone https://gitlab.freedesktop.org/gstreamer/common.git     //

git clone https://gitlab.freedesktop.org/gstreamer/gstreamer.git
cd gstreamer/
git checkout -b local_1.2.3 1.2.3
ln -s ../common
cd ../

git clone https://gitlab.freedesktop.org/gstreamer/gst-plugins-base.git  
cd gst-plugins-base 
git checkout -b local_1.2.3 1.2.3 
ln -s ../common
cd ../

git clone https://gitlab.freedesktop.org/gstreamer/gst-plugins-good.git
cd gst-plugins-good
git checkout -b local_1.2.3 1.2.3 
ln -s ../common
cd ../










//*********************************************************************************************************************************/

























