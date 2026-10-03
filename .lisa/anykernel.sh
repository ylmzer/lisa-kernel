### AnyKernel3 lisa diagnostic package
properties() { '
kernel.string=Lisa 5.4.302 diagnostic kernel by ylmzer
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
PATCH_VBMETA_FLAG=auto;
. tools/ak3-core.sh;
ui_print "Diagnostic build: Chromium fix and device compatibility are unverified.";
ui_print "KernelSU/ReSukiSU is not included in this build.";
dump_boot;
write_boot;
