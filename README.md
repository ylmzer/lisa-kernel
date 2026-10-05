# ylmzer-HyperOSPro

Xiaomi 11 Lite 5G NE (`lisa` / SM7325) için Linux **5.4.302** kernel projesi.
HyperOSPro Android 17 üzerinde cihaz uyumluluğu için geliştirilir.

## Güncel durum — 5 Ekim 2026

Cihazda Android açılışı, ekran, Wi-Fi, mobil internet, arama, iki yönde ses,
SMS, hoparlör ve titreşim doğrulandı. Chromium alt süreçlerindeki başlatma
sorunu `CAP_CHECKPOINT_RESTORE` desteğinin geri taşınmasıyla düzeltildi;
kullanıcı Chrome'un çalıştığını doğruladı.

ROM tarafındaki ayrı HIDL telefon servisi uyarlamasıyla 5G bağlantısı ve
servis geri bildirimleri doğrulandı. BonusFlaş ve ÖSYM uygulamaları da ayrı
`libmediadrm.so` düzeltmesiyle çalışıyor. Bu ROM düzeltmeleri kernel ZIP'ine
dahil değildir.

Kısa kararlılık testinde yeniden başlama, kernel panic, OOM, ANR veya bağlantı
kesilmesi gözlenmedi. Chrome/Edge, BonusFlaş ve ÖSYM arasında kullanıcı testi
sorunsuz geçti. Uzun süreli RAM, pil tüketimi, termal davranış ve ağ geçişleri
henüz kapsamlı olarak doğrulanmadı.

## Kernel ve yükleme paketi

- Yeni derlemelerde kernel sürümü: `5.4.302-HyperOSPro`.
- TWRP yükleyici adı: `ylmzer-HyperOSPro`.
- AnyKernel3 paketi: `ylmzer-HyperOSPro.zip`.
- [Son cihazda doğrulanan kernel derlemesi](https://github.com/ylmzer/lisa-kernel/actions/runs/37275763441): `d5aea08865560cc00f92f26f0041fc776003b942`.
- [Yeni adlandırmayı içeren derleme](https://github.com/ylmzer/lisa-kernel/actions/runs/37354106357): `18fee1b90ea972632ac8f50f93ce62984cb61320`; çıktı doğrulandı ve kullanıcı yükledi. Boşluklu sürüm metni Ayarlar’da `Unavailable` gösterdiği için yeni sürümlerde tire kullanılır.

Paket mevcut boot imajına yeni kerneli yerleştirir; ROM'un ramdisk, DTB ve
DTBO bileşenlerini korur. KernelSU/ReSukiSU eklenmemiştir; yükleyici vbmeta
bayraklarını değiştirmez. TWRP mesajları normal kernel kurulum adımlarını gösterir.

## Kaynak geçmişi

Temel kaynak [xiaomi-lisa-devs/android_kernel_xiaomi_sm8350](https://github.com/xiaomi-lisa-devs/android_kernel_xiaomi_sm8350)
`lineage-23.2`, commit `7ede20c8692e0e0fc044754429e33cfd1d75c40d`.
Bu temel üzerinde Lisa ve Android 17 uyarlamaları geliştirilir.

- [Proje ve derleme belgesi](.lisa/README.md)
- [Xiaomi ekran kodunun kaynağı](.lisa/mi-display-source.md)
- [LZ4KD kaynağı](.lisa/lz4kd-source.md)
- [Android açılışı / NetBpfLoad](.lisa/netbpfload-compat.md)
- [Ekran](.lisa/display-bringup.md), [ağ](.lisa/network-bringup.md), [ses](.lisa/audio-bringup.md), [titreşim](.lisa/vibration-bringup.md)

## Derleme

GitHub Actions → **Build ylmzer-HyperOSPro kernel** → **Run workflow**.
`config-only` ayar denetimi, `build` tam derleme yapar; `package` seçeneği
AnyKernel3 ZIP'ini üretir. `boot_log` ve `zygote_diag` tanılama seçenekleridir;
kurulum adı ve sürüm metnini değiştirmez.

Çıktılar arasında `Image`, `vmlinux`, `System.map`, çözülmüş ayarlar, DTB
arşivi, derleme bilgileri ve SHA-256 listesi bulunur. CI kaynak bütünlüğünü,
uyumluluk düzeltmelerini ve gerekli sürücü sembollerini denetler. Derleme
başarısı ve cihaz testi ayrı doğrulama adımlarıdır.

Ham cihaz kayıtları, kişisel bilgiler ve özel ROM ikilileri repoya eklenmez.
Linux kaynak belgeleri: [README](README), [COPYING](COPYING).
