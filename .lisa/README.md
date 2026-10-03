# Lisa kernel â€” ylmzer

Xiaomi 11 Lite 5G NE (`lisa`, SM7325) iÃ§in baÄŸÄ±msÄ±z Linux 5.4.302 kaynak reposu.
Bu repo Tachyon'un kaynak kodu deÄŸildir. KaynaklarÄ± paylaÅŸÄ±lamayan Tachyon'un
yerine, aÃ§Ä±k bir kaynak aÄŸacÄ± Ã¼zerinde dÃ¼zenlenebilir bir tanÄ± temeli sunar.

## Kaynak ve cihaz referansÄ±

- Kaynak: https://github.com/xiaomi-lisa-devs/android_kernel_xiaomi_sm8350
- Referans commit: `7ede20c8692e0e0fc044754429e33cfd1d75c40d` (`lineage-23.2`).
- BaÄŸÄ±msÄ±z repo geÃ§miÅŸi bu kaynak anlÄ±k gÃ¶rÃ¼ntÃ¼sÃ¼nden baÅŸlar; kaynak lisanslarÄ±
  ve telif bildirimleri korunur.
- Cihazdan 3 Ekim 2026'da okunan kernel: `5.4.302-tachyon #4`, Clang 18.1.8.
- ROM'un bildirdiÄŸi sÃ¼rÃ¼m: Android 17 / SDK 37, `OS4.0.0.32.XPCCNXM`.
- `configs/tachyon-reference.config`: doÄŸrudan `/proc/config.gz` Ã¼zerinden
  alÄ±nan ayarlar. AynÄ± ayarlar, aynÄ± kaynak veya aynÄ± davranÄ±ÅŸ anlamÄ±na gelmez.

## Ä°lk hedef

Chromium tabanlÄ± tarayÄ±cÄ±larda gÃ¶zlenen native sÃ¼reÃ§ baÅŸlatma sorununu,
KernelSU kancalarÄ± bulunmayan bir kernel ile karÅŸÄ±laÅŸtÄ±rmak. Ã–nceki kayÄ±tlarda
`zygote-child` SIGABRT ve ardÄ±ndan GPU kanal zaman aÅŸÄ±mÄ± gÃ¶rÃ¼lÃ¼yor. KernelSU
setuid kancasÄ± bir hipotezdir; kesin neden veya doÄŸrulanmÄ±ÅŸ dÃ¼zeltme deÄŸildir.

Bu kaynak aÄŸacÄ±nda KernelSU/ReSukiSU eklenmemiÅŸtir. Root gerektiren modÃ¼ller
ilk tanÄ± derlemesinde Ã§alÄ±ÅŸmaz. Telefonda test edilmeden uyumluluk iddiasÄ± yoktur.

## GitHub Ã¼zerinde derleme

Actions â†’ **Build Lisa diagnostic kernel** â†’ **Run workflow**.
Derleme bu repodaki kaynaklarÄ± kullanÄ±r; baÅŸka bir kernel reposunu indirmez.
Ubuntu 22.04 ve LLVM/Clang 14 ile baÅŸlar. Bu, kaynak aÄŸacÄ±nÄ±n eski CFI/LTO
arayÃ¼zÃ¼ne uygun bir baÅŸlangÄ±Ã§tÄ±r; Tachyon'un Clang 18 derlemesini taklit etmez.
Paket sÃ¼rÃ¼mleri `build-info.txt` iÃ§ine kaydedilir.

Ä°lk Ã§alÄ±ÅŸtÄ±rmada `config-only` seÃ§eneÄŸi ayar Ã§Ã¶zÃ¼mlemesini doÄŸrular.
Tam derleme `Image`, DTB'ler, `vmlinux`, `System.map`, `.config`, ayar farklarÄ±,
derleme logu ve SHA-256 listesi Ã¼retir. `package` seÃ§ilirse ayrÄ±ca AnyKernel3
tanÄ± ZIP'i Ã¼retilir. Artifakt Ã¼retimi, telefonda Ã§alÄ±ÅŸtÄ±ÄŸÄ± anlamÄ±na gelmez.

Paket yalnÄ±zca `Image` iÃ§erir; telefonun mevcut DTB/DTBO ve ramdisk'ini korur.
Yeni DTB/DTBO dosyalarÄ± otomatik flash edilmez. Ã‡alÄ±ÅŸan kernelde olduÄŸu gibi
vendor sÃ¼rÃ¼cÃ¼leri Ã§ekirdeÄŸe gÃ¶mÃ¼lÃ¼ tutulmaya Ã§alÄ±ÅŸÄ±lÄ±r; zorla modÃ¼l yÃ¼kleme
aÃ§Ä±lmaz ve `MODVERSIONS` kapatÄ±lmaz.

## Linux Ã¼zerinde yerel derleme

```sh
sudo apt-get install bc bison flex make gcc g++ libssl-dev libelf-dev \
  libncurses-dev python3 rsync zip clang-14 lld-14 llvm-14 \
  gcc-aarch64-linux-gnu gcc-arm-linux-gnueabi
bash .lisa/build.sh config-only
bash .lisa/build.sh build
```

Derleme Ã§Ä±ktÄ±larÄ± `out/lisa` ve `dist/lisa` altÄ±ndadÄ±r. Derleme betiÄŸi
kaynak aÄŸacÄ±nÄ± deÄŸiÅŸtirmez. `config-diff.txt` iÃ§indeki donanÄ±m ayarÄ± farklarÄ±
ve derleme uyarÄ±larÄ± ilk cihaz testinden Ã¶nce incelenmelidir.

## Cihaz testi

Ã‡alÄ±ÅŸan ROM'a ait boot yedeÄŸi ve geri dÃ¶nÃ¼ÅŸ yÃ¶ntemi hazÄ±rlandÄ±ktan sonra tanÄ±
kernelini test et. AÃ§Ä±lÄ±ÅŸ, ekran, dokunmatik, Wi-Fi, mobil veri, arama, ses,
kamera, uyku/uyanma ve ÅŸarjÄ± kontrol et. ArdÄ±ndan aynÄ± Chromium sÃ¼rÃ¼mÃ¼yle
aÃ§Ä±lÄ±ÅŸ, sayfa yÃ¼kleme ve video oynatmayÄ± dene; logcat crash ve kernel logunu
Ã¶nceki kayÄ±tlarla karÅŸÄ±laÅŸtÄ±r. TarayÄ±cÄ± sorunu devam ederse Android 17
`zygote_next` / `libzygote_sys` ve vendor kullanÄ±cÄ± alanÄ± ayrÄ±ca incelenmelidir.

Ham cihaz loglarÄ± ve kiÅŸisel bilgiler bu repoya dahil edilmez.
