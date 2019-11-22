zlib 编译
git clone https://github.com/madler/zlib.git          V=1.2.8
修改Makefile

ffilib编译
下载 https://sourceware.org/libffi/      V=3.2.1
./configure  CC=/opt/ti_components/os_tools/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --host=arm-linux --prefix=/home/taohaiwu/works/project/adas/c211/workspace/rootfs/usr
make

sudo make install


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

./configure --prefix=/home/taohaiwu/works/project/adas/c211/workspace/rootfs/usr  LFLAGS="-lrt" CC=/opt/ti_components/os_tools/linaro/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc --host=arm-linux LIBFFI_CFLAGS="-I/usr/local/C211_5.7/lib/libffi-3.2.1/include" LIBFFI_LIBS="-lffi -L/usr/local/C211_5.7/lib" ZLIB_CFLAGS="-I/usr/local/C211_5.7/include" ZLIB_LIBS="-lz -L/usr/local/C211_5.7/lib"   --cache-file=glib.cache   --disable-selinux  --disable-xattr --disable-libelf CXXFLAGS="-lrt" CFLAGS="-lrt"


















