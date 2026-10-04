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

Güncel öncelik Android açılışını sağlamaktır. İlk kernel testleri logo
döngüsüne girdi. 4 Ekim bootdiag testi NetBpfLoad'un 5.10 sürüm kontrolünü
ilk engel olarak kaydetti; çalışan Tachyon'daki süreç bazlı uyumluluk davranışı
doğrulandı ve kaynakta eklendi. [Açılış bulguları](compatibility.md) ve
[düzeltmenin kapsamı](netbpfload-compat.md) ayrıca kayıtlıdır. Yeni paketin
cihazda açılışı doğrulanana kadar Chromium çalışması bekletilir.

Chromium tabanlı tarayıcılarda gözlenen native süreç başlatma sorununu,
KernelSU kancaları bulunmayan bir kernel ile karşılaştırmak. Önceki kayıtlarda
`zygote-child` SIGABRT ve ardından GPU kanal zaman aşımı görülüyor. KernelSU
setuid kancası bir hipotezdir; kesin neden veya doğrulanmış düzeltme değildir.

Bu kaynak ağacında KernelSU/ReSukiSU eklenmemiştir. Root gerektiren modüller
ilk tanı derlemesinde çalışmaz. Telefonda test edilmeden uyumluluk iddiası yoktur.
İlk GitHub ayar denetimi ve [tam kernel derlemesi](https://github.com/ylmzer/lisa-kernel/actions/runs/37129546878)
başarılıdır. `3a12e2b7a58f89c0f332f82978ee3de21efc7557` kaynak commit'inden
`5.4.302-lisa-ylmzer+` Image ve tanı ZIP'i üretildi. İndirilen çıktılarda
SHA-256, ARM64 Image/ELF biçimi, gerekli ayarlar, son kerneldeki ekran bildirim
fonksiyonları ve ZIP içindeki Image eşleşmesi doğrulandı.
Mevcut ROM'un kullandığı `lz4kd` desteği eklendi; dört sayfa sıkıştırma/açma
testi ve tam derleme geçti. Bunlar cihazda açılış veya Chromium testi değildir. Diğer Xiaomi ekleri
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

Tanı paketinde AnyKernel3'ün otomatik Magisk algılama/yama yolu kapalıdır
(`NO_MAGISK_CHECK=1`); vbmeta bayraklarını değiştirme seçeneği de kapalıdır.
Mevcut boot içeriği yeniden paketlenir ve yeni kernel imajı yerleştirilir.

## Linux üzerinde yerel derleme

Kaynakları Linux dosya sisteminde bu repodan yeniden klonlayın. Windows Git
çalışma kopyasında sembolik bağlantılar metin dosyası olarak temsil edilir;
Linux klonu gerçek bağlantıları geri kurar.

```sh
sudo apt-get install bc bison flex make gcc g++ libssl-dev libelf-dev \
  libncurses-dev python3 rsync zip clang-14 lld-14 llvm-14 \
  gcc-aarch64-linux-gnu gcc-arm-linux-gnueabi
git clone https://github.com/ylmzer/lisa-kernel.git
cd lisa-kernel
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
Başlangıç bulguları ve karşılaştırma adımları [Chromium test planında](chromium.md)
ayrıntılandırılmıştır.

Ham cihaz logları ve kişisel bilgiler bu repoya dahil edilmez.

## Kaynak bütünlüğü ve derleme düzeltmeleri

Linux kaynaklarında yalnızca harf büyüklüğü farklı olan dosyalar bulunur
(`xt_MARK.h` / `xt_mark.h` gibi). Windows çalışma kopyası bu dosyaları ayrı
saklayamaz. Git ağacındaki 13 çift, sabitlenmiş upstream arşivinden içerikleri
değiştirilmeden geri alınmıştır. CI, 26 dosyanın Git blob SHA-1 değerlerini
`.lisa/tests/source_integrity.py` ile derlemeden önce doğrular.
Bu Windows kopyasında ilgili yollar `skip-worktree` olarak işaretlidir;
derlemek veya bu dosyaları düzenlemek için Linux dosya sisteminde yeni bir
clone kullanın. Windows üzerinden kaynak ağacının tamamını yeniden eklemeyin.

LTO'nun kullandığı `LLVM_NM` değişkeni derleyiciyle aynı sürüme açıkça
sabitlenmiştir. Wi-Fi sürücüsünde `wlan_hdd_state_ctrl_param_create` fonksiyonu
yalnızca çağrıldığı modül yapılandırmasında derlenir; yerleşik sürücü yolu ve
derleyicinin hata denetimleri korunur.

Yerleşik Wi-Fi derlemesinde IPA ve netlink yardımcıları da kernelin genel
fonksiyonlarıyla isim çakıştırıyordu. Wi-Fi'nin kendi hazır olma kontrolü
`wlan_ipa_is_ready`, CLD80211 başlık yardımcısı `cld80211hdr_put` olarak
adlandırıldı; bildirimler ve ilgili çağrılar birlikte güncellendi. Platformun
`ipa_is_ready` API'si ve kernelin NL80211 kodu değiştirilmedi.

Vendor ekran, ses ve kamera seçenekleri yalnızca `.config` içinde değildir;
techpack Makefile profilleri ayrıca `m` seçiyordu. Lisa'nın lahaina/yupik
profilleri yerleşik derlemeye alındı. Genel DRM sürücüsünü açmak yerine
Qualcomm techpack ekran sağlayıcısı derlenir. `techpack_audit.py` bu ayrı
profilleri denetler; tam derlemeden önce vendor dizinleri derlenir ve Xiaomi
ekran bildirim sembollerinin üretildiği doğrulanır. Bunlar cihaz testi yerine
geçmez. CI derleyici önbelleğini başarısız çalışmalarda da saklar; sonraki
düzeltmeler aynı nesneleri yeniden derlemek zorunda kalmaz.

Windows'ta test öncesi ve sonrası kayıt almak için:

```powershell
& .\.lisa\capture-device.ps1
```

Betiğin yaptığı işlemler yalnızca cihaz bilgilerini okumak ve yerel dosyalara
kaydetmektir. Sonuçlar Git tarafından dışlanan `.lisa/private/captures` altında
tutulur. `-AdbPath` ile ADB yolu, `-OutputDirectory` ile farklı kayıt konumu
belirtilebilir. Farklı bir konum seçilirse kayıtların gizliliğini ayrıca koruyun.
