# Lisa kernel — ylmzer

Xiaomi 11 Lite 5G NE (`lisa`, SM7325) için bağımsız Linux 5.4.302 kaynak reposu.
Bu repo Tachyon'un kaynak kodu değildir. Kaynakları paylaşılamayan Tachyon'un
yerine, açık bir kaynak ağacı üzerinde düzenlenebilir bir tanı temeli sunar.

## Kaynak ve cihaz referansı

- Kaynak: https://github.com/xiaomi-lisa-devs/android_kernel_xiaomi_sm8350
- Referans commit: `7ede20c8692e0e0fc044754429e33cfd1d75c40d` (`lineage-23.2`).
- Bağımsız repo geçmişi bu kaynak anlık görüntüsünden başlar; kaynak lisansları
  ve telif bildirimleri korunur.
- Cihazdan 3 Ekim 2026'da okunan kernel: `5.4.302-tachyon #4`, Clang 18.1.8.
- ROM'un bildirdiği sürüm: Android 17 / SDK 37, `OS4.0.0.32.XPCCNXM`.
- `configs/tachyon-reference.config`: doğrudan `/proc/config.gz` üzerinden
  alınan ayarlar. Aynı ayarlar, aynı kaynak veya aynı davranış anlamına gelmez.

## İlk hedef

Chromium tabanlı tarayıcılarda gözlenen native süreç başlatma sorununu,
KernelSU kancaları bulunmayan bir kernel ile karşılaştırmak. Önceki kayıtlarda
`zygote-child` SIGABRT ve ardından GPU kanal zaman aşımı görülüyor. KernelSU
setuid kancası bir hipotezdir; kesin neden veya doğrulanmış düzeltme değildir.

Bu kaynak ağacında KernelSU/ReSukiSU eklenmemiştir. Root gerektiren modüller
ilk tanı derlemesinde çalışmaz. Telefonda test edilmeden uyumluluk iddiası yoktur.
İlk GitHub ayar denetimi başarılıdır. Mevcut ROM'un kullandığı `lz4kd` desteği
eklendi; codec testi ve tam derleme ayrıca doğrulanıyor. Diğer Xiaomi ekleri
için [uyumluluk farklarına](compatibility.md) bakın.

## GitHub üzerinde derleme

Actions → **Build Lisa diagnostic kernel** → **Run workflow**.
Derleme bu repodaki kaynakları kullanır; başka bir kernel reposunu indirmez.
Ubuntu 22.04 ve LLVM/Clang 14 ile başlar. Bu, kaynak ağacının eski CFI/LTO
arayüzüne uygun bir başlangıçtır; Tachyon'un Clang 18 derlemesini taklit etmez.
Paket sürümleri `build-info.txt` içine kaydedilir.

İlk çalıştırmada `config-only` seçeneği ayar çözümlemesini doğrular.
Tam derleme `Image`, DTB'ler, `vmlinux`, `System.map`, `.config`, ayar farkları,
derleme logu ve SHA-256 listesi üretir. `package` seçilirse ayrıca AnyKernel3
tanı ZIP'i üretilir. Artifakt üretimi, telefonda çalıştığı anlamına gelmez.

Paket yalnızca `Image` içerir; telefonun mevcut DTB/DTBO ve ramdisk'ini korur.
Yeni DTB/DTBO dosyaları otomatik flash edilmez. Çalışan kernelde olduğu gibi
vendor sürücüleri çekirdeğe gömülü tutulmaya çalışılır; zorla modül yükleme
açılmaz ve `MODVERSIONS` kapatılmaz.

## Linux üzerinde yerel derleme

```sh
sudo apt-get install bc bison flex make gcc g++ libssl-dev libelf-dev \
  libncurses-dev python3 rsync zip clang-14 lld-14 llvm-14 \
  gcc-aarch64-linux-gnu gcc-arm-linux-gnueabi
bash .lisa/build.sh config-only
bash .lisa/build.sh build
```

Derleme çıktıları `out/lisa` ve `dist/lisa` altındadır. Derleme betiği
kaynak ağacını değiştirmez. `config-diff.txt` içindeki donanım ayarı farkları
ve derleme uyarıları ilk cihaz testinden önce incelenmelidir.

## Cihaz testi

Çalışan ROM'a ait boot yedeği ve geri dönüş yöntemi hazırlandıktan sonra tanı
kernelini test et. Açılış, ekran, dokunmatik, Wi-Fi, mobil veri, arama, ses,
kamera, uyku/uyanma ve şarjı kontrol et. Ardından aynı Chromium sürümüyle
açılış, sayfa yükleme ve video oynatmayı dene; logcat crash ve kernel logunu
önceki kayıtlarla karşılaştır. Tarayıcı sorunu devam ederse Android 17
`zygote_next` / `libzygote_sys` ve vendor kullanıcı alanı ayrıca incelenmelidir.

Ham cihaz logları ve kişisel bilgiler bu repoya dahil edilmez.
