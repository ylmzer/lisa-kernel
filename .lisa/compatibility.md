# HyperOS port uyumluluğu: doğrulanmış farklar

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
açılış, donanım ve tarayıcı davranışı henüz test edilmemiştir.
