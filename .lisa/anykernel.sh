### AnyKernel3 ylmzer-HyperOSPro package
properties() { '
kernel.string=ylmzer-HyperOSPro
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
ui_print "Kernel surumu: 5.4.302 HyperOSPro";
ui_print "Mevcut boot imaji okunuyor...";
dump_boot;
ui_print "Yeni kernel boot imajina ekleniyor ve yaziliyor...";
write_boot;
ui_print "Kernel kurulumu tamamlandi.";
