zlib 编译
git clone https://github.com/madler/zlib.git          V=1.2.8
修改Makefile

ffilib编译
下载 https://sourceware.org/libffi/      V=3.2.1
./configure  CC=/opt/ti_components/os_tools/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --host=arm-linux --prefix=/home/taohaiwu/works/project/adas/c211/rootfs/targetfs/usr
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

./configure --prefix=/home/taohaiwu/works/project/adas/c211/rootfs/targetfs/usr   CC=/opt/ti_components/os_tools/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --host=arm-linux LIBFFI_CFLAGS="-I/usr/local/C211_5.7/lib/libffi-3.2.1/include" LIBFFI_LIBS="-lffi -L/usr/local/C211_5.7/lib" ZLIB_CFLAGS="-I/usr/local/C211_5.7/include" ZLIB_LIBS="-lz -L/usr/local/C211_5.7/lib"   --cache-file=glib.cache   --disable-selinux  --disable-xattr --disable-libelf 
 make 
 make install
 
 
 //ofilm   gstreamer 编译
cd workspace/dvr

cd workspace/dvr/build 
/********************************envsetup.sh   diff ******************************************************************************************************/
diff --git a/build/envsetup.sh b/build/envsetup.sh
index 52d35e0..2e78f64 100755
--- a/build/envsetup.sh
+++ b/build/envsetup.sh
@@ -6,7 +6,7 @@
 ##
 ##
 #PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-
-PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
+PREFIX=/opt/ti_components/os_tools/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
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
+export PATH=$PATH:/opt/ti_components/os_tools/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin
////**********************************************************************************************************************************/


////***************************************************************Makefile.Inc   diff *******************************************************************/
taohaiwu@ubuntu:~/works/project/adas/c211/workspace/dvr$ git diff Makefile.Inc
diff --git a/Makefile.Inc b/Makefile.Inc
index d25bf31..5c061de 100755
--- a/Makefile.Inc
+++ b/Makefile.Inc
@@ -3,7 +3,7 @@
 ##COMPILE tools
 ##
 ##
-PREFIX=/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-
+PREFIX=/opt/ti_components/os_tools/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
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
////**********************************************************************************************************************************/



//************编译ubuntu gstreamer****************************************************************************************************/



//*********************************************************************************************************************************/

























