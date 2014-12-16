#!/sbin/sh

# decompress ramdisk
mkdir /tmp/ramdisk
cd /tmp/ramdisk
gunzip -c ../boot.img-ramdisk.gz | cpio -i

# add init.d support if not already supported
found=$(find init.rc -type f | xargs grep -oh "run-parts /system/etc/init.d");
if [ "$found" != 'run-parts /system/etc/init.d' ]; then
        #find busybox in /system
        bblocation=$(find /system/ -name 'busybox')
        if [ -n "$bblocation" ] && [ -e "$bblocation" ] ; then
                echo "BUSYBOX FOUND!";
                #strip possible leading '.'
                bblocation=${bblocation#.};
        else
                echo "NO BUSYBOX NOT FOUND! init.d support will not work without busybox!";
                echo "Setting busybox location to /system/xbin/busybox! (install it and init.d will work)";
                #set default location since we couldn't find busybox
                bblocation="/system/xbin/busybox";
        fi
		#append the new lines for this option at the bottom
        echo "" >> init.rc
        echo "service userinit $bblocation run-parts /system/etc/init.d" >> init.rc
        echo "    oneshot" >> init.rc
        echo "    class late_start" >> init.rc
        echo "    user root" >> init.rc
        echo "    group root" >> init.rc
fi

# Modify data mode with writeback instead default mount option
#line_no=$(sed -n "/by-name\/system/ =" fstab.mako)
#sed -i "$line_no c\/dev\/block\/platform\/msm_sdcc.1\/by-name\/system       \/system         ext4    ro,barrier=1,data=writeback                                      wait" fstab.mako
#line_no=$(sed -n "/by-name\/cache/ =" fstab.mako)
#sed -i "$line_no c\/dev\/block\/platform\/msm_sdcc.1\/by-name\/cache        \/cache          ext4    noatime,nosuid,nodev,barrier=1,data=writeback                    wait,check" fstab.mako
#line_no=$(sed -n "/by-name\/userdata/ =" fstab.mako)
#sed -i "$line_no c\/dev\/block\/platform\/msm_sdcc.1\/by-name\/userdata     \/data           ext4    noatime,nosuid,nodev,barrier=1,data=writeback,noauto_da_alloc    wait,check,encryptable=\/dev\/block\/platform\/msm_sdcc.1\/by-name\/metadata" fstab.mako

find . | cpio -o -H newc | gzip > ../newramdisk.cpio.gz
cd /
