# HyperOS bellek genişletme ABI incelemesi

## Gerçek tüketici
Cihazdan alınan /system_ext/framework/miui-services.jar DEX kodu incelendi. ExtendMImpl.startExtM önce isKernelSupported ile /sys/block/zram0/backing_dev değerinin none dışında olmasını ister; sonra persist.miui.extm.enable=1 koşulu aranır. Yalnız CONFIG_ZRAM_WRITEBACK açmak bu koşulları sağlamaz.

getPagesNeedFlush/isFlushablePagesExist, idle_stat içindeki boşlukla ayrılmış seviye sayaçlarını okur. runMark, IVold.runExtMMark çağrısını reflection ile bulur. runFlush sayfa sayısı ve seviyeyi hesaplayarak IVold üzerinden çalışır. framework.jar IVold/Stub/Proxy tablolarında runExtMMark, runExtMFlush, stopExtMFlush ve getPagesNeedFlush adları mevcut. Bu, vold native uygulamasının doğru çalıştığını tek başına kanıtlamaz.

MFZ, isMfzSupported içinde mfz_enable varlığıyla ayrı kapılanır; startExtM içindeki normal init yolundan sonra opsiyonel kola girilir. Bu nedenle bütün MFZ/QPaCE/zgroup özellikleri normal bellek genişletme için zorunluymuş gibi sunulmamalıdır.

MiuiSwapService.swapControl başarılı/başarısız iş sonunda persist.sys.swapservice.ctrl=false yazıyor. Bu özelliğin o anda false olması yalnız başına kalıcı devre dışı olduğunu kanıtlamaz. Önceki genel yorum bu nedenle daraltılmıştır.

## Uygun kaynak bulundu
MiCode/Xiaomi_Kernel_OpenSource lisa-r-oss, sabit commit 57ee2b068000ac79caa7c076d0d9bb1a9260e8a4:
- zram_drv.c blob dd1df1f3bb95edc51c950dda5bdec2f3a2a9eb5f
- gerçek idle sayaçları ve 10 seviyelik idle_stat
- idle all işaretlemesinde artan, erişim/yeni sayfa/reset yollarında temizlenen sayaçlar
- new all ile idle bayrağı ve yaş sayaçlarının sıfırlanması
- writeback: idle <max_pages> <minimum_idle_count>; standart huge biçimi korunuyor

Sayaçları olmadan sabit sıfırlı idle_stat eklemek veya istemleri başarılı gösteren sysfs stubları eklemek doğru değildir. Aktarımda slot kilitleri, writeback sırasında erişilen sayfanın iptali, sayaç sıfırlama yolları ve flags alanının bit genişliği birlikte ele alınmalıdır. Mevcut ağaç FLAG_SHIFT=24, Lisa donor PAGE_SHIFT+1 kullanıyor; header toptan kopyalanmamalıdır. ARM32/ARM64 bit yerleşimi derleme zamanında doğrulanmalıdır.

## Kalan uygulama sırası
1. Yaş/idle sayacı, new/idle_stat ve sınırlı writeback ayrıştırıcısını birlikte ve dar kapsamda taşı.
2. Gerçek yardımcı kod üzerinde seviye sınırı, doyma, reset, mevcut flags korunması ve geçersiz girdi testleri; sonra arm64 ZRAM derlemesi.
3. ROM vold native uygulamasını ve backing_dev kuruluş yolunu doğrula. Donor fstab/backing ayarını körlemesine kopyalama.
4. Tam kernel, enforcing izinleri ve boşta/yük altında geri-okuma testi. Kullanıcının verisi üzerinde swap reset/swapoff yapma.

Mevcut tam derleme 39140439782 yalnız standart writeback içindir. Bu belge bir OEM aktarımının uygulanmış veya bitmiş olduğunu iddia etmez. Telefon değiştirilmedi.
