# HyperOS port uyumluluğu: doğrulanmış farklar

## 4 Ekim 2026: ilk Android açılış engeli kaydedildi

Bootdiag kaynak sürümü `c18e4724` cihazda denendi; logo döngüsü yeniden oluştu.
Oops bölümünden üç açılışa ait altı geçerli snapshot alındı. İki Android
denemesinde `NetBpfLoad: Android 25Q4 requires kernel 5.10.` mesajını sistem
kapatma işlemleri izliyor. Yüklü loader bu kontrolden 7 hata koduyla çıkıyor;
APEX servis tanımı başarısızlıkta `reboot,bpfloader-failed` istiyor.

Çalışan Tachyon, yalnızca belirli root süreçlerinin uname yanıtını 5.10.248
olarak değiştiriyor. NetBpfLoad için bu davranış hem kernel imajının makine
kodunda hem çalışan Android günlüğünde doğrulandı. Gerçek kernel 5.4.302,
NetBpfLoad'un gördüğü sürüm 5.10.248 ve `bpf.progs_loaded=1`.

`22655521f` kaynak sürümü root `netbpfload` için bu uyumluluk davranışını
ekler. Kapsam/test ayrıntıları [NetBpfLoad uyumluluk kaydında](netbpfload-compat.md).
Yeni paket kullanıcı tarafından yüklendi. İki açılışta `bpf.progs_loaded=1`
ve `sys.boot_completed=1` doğrulandı; logo döngüsündeki ilk engel aşıldı.
Bu pakette animasyondan sonra fiziksel ekran siyah kaldı. Sonraki `ab4f5ee5`
MIUI ekran paketiyle kullanıcı görünür ekranın başarıyla geldiğini doğruladı.
[Ekran incelemesi](display-bringup.md) kayıtları içerir. Wi-Fi, mobil veri ve
aramalar çalışmıyor; [ağ sürücüsü düzeltmeleri](network-bringup.md) derleniyor.
Chrome incelemesi temel cihaz işlevleri doğrulanana kadar bekletilir.

## İlk cihaz testi: açılış başarısız

Kullanıcı ilk tanı ZIP'ini yükledikten sonra boot logosunda tekrar tekrar
yeniden başlama bildirdi. Derleme başarısı mevcut ROM ile açılış uyumluluğu
sağlamadı. Kernel panic, kullanıcı alanının yeniden başlatması veya paket/boot
uyumsuzluğu henüz ayırt edilmedi; pstore/ramoops ve TWRP kurulum kayıtları
alınmadan kesin neden söylenemez. Bu ZIP çalışan bir kernel olarak kullanılmamalı.
İlk adım kurulum öncesi Boot yedeğine dönmektir. Chromium testi yapılamadı.

## İkinci cihaz testi: recovery açılıyor, Android açılmıyor

3 Ekim 2026 ikinci kurulum canlı izlendi. ZIP SHA-256 ve kurulum sonrası
boot_a içindeki Image SHA-256 doğrulanmış CI çıktılarıyla eşleşti. TWRP
kurulum hedefi boot_a, updater RC=0. Kullanıcı System açılışında yeniden
başlamalar bildirdi ve recovery'yi kendisi açtı. Yeni TWRP oturumunda çalışan
kernel `5.4.302-lisa-ylmzer+` olarak doğrulandı: recovery açılışı başarılı,
Android sistem açılışı başarısızdır.

Kurulum öncesi/sonrası boot görüntüleri ve TWRP logları yerel private alanda
saklandı. Ramdiskte mevcut dosyaların içerikleri korunmuş; symlink metadata
ve CPIO düzeni değişmiştir. Pstore boş ve last_kmsg yok. Oops bölümündeki
eski kayıtlar bu testin panic nedeni olarak kabul edilmedi. Normal Android
açılışının ilk hatası henüz elde edilemedi; Chrome karşılaştırması yapılmadı.

3 Ekim 2026'da ilk `config-only` GitHub çalışması başarılı oldu:
https://github.com/ylmzer/lisa-kernel/actions/runs/37119973231

Bu sonuç temel sürücü seçeneklerinin korunduğunu gösterir. Tam Tachyon veya
HyperOS uyumluluğunu göstermez. Ayar karşılaştırmasında aşağıdaki etkin Tachyon
seçeneklerinin bu açık kaynak ağacında karşılığı bulunmadı:

- `CONFIG_CRYPTO_LZ4KD`, `CONFIG_LZ4KD_COMPRESS`, `CONFIG_LZ4KD_DECOMPRESS`
  ilk temelde yoktu; aşağıda belirtilen açık kaynak uygulamayla eklendi.
- `CONFIG_MILLET`, `CONFIG_MIHW`, `CONFIG_MI_RECLAIM`, `CONFIG_CAM_RECLAIM`.
- `CONFIG_BOARD_XIAOMI_LISA`, `CONFIG_BOARD_XIAOMI_YUPIK`,
  `CONFIG_USE_COMMON_CAMERA` ve bazı OEM board seçenekleri.
- Speculative page fault ve bazı derleyiciye bağlı seçenekler.
- KernelSU seçenekleri; ilk tanı sürümünde root desteği bilinçli olarak yoktur.

Telefonun `/sys/block/zram0/comp_algorithm` çıktısında etkin algoritma `lz4kd`.
Mevcut ZRAM swap alanı yaklaşık 8 GiB. Genel LZ4KD kodu, crypto arayüzü ve ZRAM
backend bağlantısı bu repoya eklendi; [kaynak kaydı ve test kapsamı](lz4kd-source.md)
ayrıca belgelendi. Kod ve ayarların derlenmesi, cihazda swap'ın başarıyla
kurulduğunu kanıtlamaz. MiCode'un orijinal `lisa-r-oss` dalında da
`lib` ve `crypto` dizinlerinde `lz4kd` bulunmadı; kaynak adı varsayılamaz.

Diğer OEM seçenekleri için sadece ayar satırı eklemek yeterli değildir.
Karşılık gelen kod, bağımlılıklar ve ROM'un beklediği kullanıcı alanı
arayüzleri incelenmelidir. Aynı isimli ayarın başka bir cihazın kaynaklarında
bulunması, lisa için uyumluluk kanıtı değildir.

Chromium sorunu için native zygote çökmesi gözlendi; belirli bir kernel
fonksiyonunun neden olduğu gösterilmedi. Yeni kaynak ağacı ve derleyici
Tachyon'dan farklı olduğundan, rootsuz derlemede tarayıcının çalışması tek
başına KernelSU'nun hatanın nedeni olduğunu kanıtlamaz.

Bu repo tam kaynak ve derleme altyapısı sağlar. İlk ZIP bir tanı çıktısıdır;
telefonun günlük kullanım kernelinin yerine geçtiği henüz doğrulanmamıştır.

İlk başarılı tam derleme: https://github.com/ylmzer/lisa-kernel/actions/runs/37129546878
Kaynak commit: `3a12e2b7a58f89c0f332f82978ee3de21efc7557`.
Techpack ekran, ses ve kamera profilleri yerleşik olarak derlendi. Genel DRM
sürücüsünü ayrıca açmadan Xiaomi ekran bildirim fonksiyonları son kernelde
doğrulandı. LTO, son bağlama, codec testi ve ZIP üretimi başarılıdır. Telefonda
açılış testi başarısızdır; donanım ve tarayıcı davranışı doğrulanamamıştır.
