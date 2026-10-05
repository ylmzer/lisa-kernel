### AnyKernel3 ylmzer-HyperOSPro package
properties() { '
kernel.string=ylmzer-HyperOSPro - 5.4.302
do.devicecheck=1
do.modules=0
do.systemless=0
do.cleanup=1
do.cleanuponabort=0
device.name1=lisa
supported.versions=
supported.patchlevels=
supported.vendorpatchlevels=
'; }
BLOCK=boot;
IS_SLOT_DEVICE=1;
RAMDISK_COMPRESSION=auto;
PATCH_VBMETA_FLAG=0;
NO_MAGISK_CHECK=1;
. tools/ak3-core.sh;
ui_print "ylmzer-HyperOSPro kernel kuruluyor...";
ui_print "Xiaomi 11 Lite 5G NE (lisa) - 5.4.302";
dump_boot;
write_boot;
