# linux-exec-trace
trace new executed processes


##Usage1:
Download linux-exec-trace from releases and run it

##Usage2:
```
docker run --cap-add NET_ADMIN --userns host --user 0 --pid host --network host osexp2000/linux-exec-trace linux-exec-trace
```


###Sample output

```
6199 21047 166536:166536 /bin/bash /home/devuser/android-gcc-toolchain/android-gcc-toolchain --api max --stl libc++ gcc -o a a.c
6200 6199 166536:166536 /bin/readlink(readlink) /home/devuser/android-gcc-toolchain/android-gcc-toolchain
6199 21047 166536:166536 /bin/bash /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/gcc -o a a.c
6199 21047 166536:166536 /usr/bin/ccache(ccache) /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/arm-linux-androideabi-gcc -o a a.c
6199 21047 166536:166536 /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/arm-linux-androideabi-gcc -o a a.c
6201 6199 166536:166536 /home/devuser/ndk/std-toolchains/android-24-arm-libc++/libexec/gcc/arm-linux-androideabi/4.9.x/cc1(/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../libexec/gcc/arm-linux-androideabi/4.9.x/cc1) -quiet -iprefix /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc/arm-linux-androideabi/4.9.x/ -isysroot /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot a.c -mbionic -fpic -quiet -dumpbase a.c '-march=armv5te' '-mfloat-abi=soft' '-mfpu=vfp' '-mtls-dialect=gnu' -auxbase a -o /tmp/ccZVSe5a.s
6202 6199 166536:166536 /home/devuser/ndk/std-toolchains/android-24-arm-libc++/arm-linux-androideabi/bin/as(/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc/arm-linux-androideabi/4.9.x/../../../../arm-linux-androideabi/bin/as) '-march=armv5te' '-mfloat-abi=soft' '-mfpu=vfp' '-meabi=5' --noexecstack -o /tmp/cchgNklt.o /tmp/ccZVSe5a.s
6203 6199 166536:166536 /home/devuser/ndk/std-toolchains/android-24-arm-libc++/libexec/gcc/arm-linux-androideabi/4.9.x/collect2(/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../libexec/gcc/arm-linux-androideabi/4.9.x/collect2) -plugin /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../libexec/gcc/arm-linux-androideabi/4.9.x/liblto_plugin.so '-plugin-opt=/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../libexec/gcc/arm-linux-androideabi/4.9.x/lto-wrapper' '-plugin-opt=-fresolution=/tmp/cc1OJECL.res' '-plugin-opt=-pass-through=-lgcc' '-plugin-opt=-pass-through=-lc' '-plugin-opt=-pass-through=-ldl' '-plugin-opt=-pass-through=-lgcc' '--sysroot=/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot' --eh-frame-hdr -dynamic-linker /system/bin/linker -X -m armelf_linux_eabi -z noexecstack -z relro -z now -o a /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot/usr/lib/crtbegin_dynamic.o -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc/arm-linux-androideabi/4.9.x -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc/arm-linux-androideabi/4.9.x/../../../../arm-linux-androideabi/lib -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot/usr/lib /tmp/cchgNklt.o -lgcc -lc -ldl -lgcc /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot/usr/lib/crtend_android.o
6204 6203 166536:166536 /home/devuser/ndk/std-toolchains/android-24-arm-libc++/arm-linux-androideabi/bin/ld(/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc/arm-linux-androideabi/4.9.x/../../../../arm-linux-androideabi/bin/ld) -plugin /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../libexec/gcc/arm-linux-androideabi/4.9.x/liblto_plugin.so '-plugin-opt=/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../libexec/gcc/arm-linux-androideabi/4.9.x/lto-wrapper' '-plugin-opt=-fresolution=/tmp/cc1OJECL.res' '-plugin-opt=-pass-through=-lgcc' '-plugin-opt=-pass-through=-lc' '-plugin-opt=-pass-through=-ldl' '-plugin-opt=-pass-through=-lgcc' '--sysroot=/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot' --eh-frame-hdr -dynamic-linker /system/bin/linker -X -m armelf_linux_eabi -z noexecstack -z relro -z now -o a /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot/usr/lib/crtbegin_dynamic.o -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc/arm-linux-androideabi/4.9.x -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../lib/gcc/arm-linux-androideabi/4.9.x/../../../../arm-linux-androideabi/lib -L/home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot/usr/lib /tmp/cchgNklt.o -lgcc -lc -ldl -lgcc /home/devuser/ndk/std-toolchains/android-24-arm-libc++/bin/../sysroot/usr/lib/crtend_android...
```

