#!/sbin/sh
#repacks the ramdisk

mkdir /tmp/ramdisk
cp /tmp/boot.img-ramdisk.gz /tmp/ramdisk/
cd /tmp/ramdisk/
gunzip -c /tmp/ramdisk/boot.img-ramdisk.gz | cpio -i
cd /
rm /tmp/ramdisk/boot.img-ramdisk.gz
rm /tmp/boot.img-ramdisk.gz
cd /tmp/ramdisk/ 
 #remove governor overrides, use kernel default
sed -i '/\/sys\/devices\/system\/cpu\/cpu0\/cpufreq\/scaling_governor/d' /tmp/ramdisk/init.palman.rc
sed -i '/\/sys\/devices\/system\/cpu\/cpu1\/cpufreq\/scaling_governor/d' /tmp/ramdisk/init.palman.rc
sed -i '/\/sys\/devices\/system\/cpu\/cpu2\/cpufreq\/scaling_governor/d' /tmp/ramdisk/init.palman.rc
sed -i '/\/sys\/devices\/system\/cpu\/cpu3\/cpufreq\/scaling_governor/d' /tmp/ramdisk/init.palman.rc  
sed -i "/\b\(setprop ro.tether.denied true\)\b/d" init.palman.rc
sed -i '/write \/proc\/sys\/vm\/dirty_expire_centisecs 200/c \   write /proc/sys/vm/dirty_expire_centisecs 300' init.rc
sed -i '/write \/proc\/sys\/vm\/dirty_background_ratio  5/c \    write /proc/sys/vm/dirty_background_ratio  10' init.rc
find . | cpio -o -H newc | gzip > ../boot.img-ramdisk.gz
cd /
rm -rf /tmp/ramdisk



