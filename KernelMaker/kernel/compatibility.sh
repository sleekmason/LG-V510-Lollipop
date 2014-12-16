#!/sbin/sh
#

# reload the old binaries
if [ -e /system/bin/mpdecision_bck ] ; then
	busybox mv /system/bin/mpdecision_bck /system/bin/mpdecision
fi
if [ -e /system/bin/thermald_bck ] ; then
	busybox mv /system/bin/thermald_bck /system/bin/thermald
fi

# reload the old library
if [ -e /system/lib/hw/power.msm8960.so_bck ] ; then
        busybox mv /system/lib/hw/power.msm8960.so_bck /system/lib/hw/power.msm8960.so
fi

# reload the old prima stuff
if [ -e /system/vendor/firmware/wlan/prima/WCNSS_cfg.dat_bck ] ; then
        busybox mv /system/vendor/firmware/wlan/prima/WCNSS_cfg.dat_bck /system/vendor/firmware/wlan/prima/WCNSS_cfg.dat
fi
if [ -e /system/etc/wifi/WCNSS_qcom_cfg.ini_bck ] ; then
        busybox mv /system/etc/wifi/WCNSS_qcom_cfg.ini_bck /system/etc/wifi/WCNSS_qcom_cfg.ini
fi
return $?
