# Android açılış tanısı

İkinci cihaz testinde repo kerneli TWRP'yi başlattı, fakat Android açılışı
yeniden başlama döngüsüne girdi. Pstore boş ve last_kmsg yoktu. ROM'un cache
arşivleri eski qgki/Tachyon denemelerine ait; güncel `.partial` arşivi boştu.
Bu yüzden henüz doğrulanmış bir açılış yaması yoktur.

ROM vendor_boot ramdiskinde 46 modüllük bir başlangıç listesi var. Kurulum
öncesi Tachyon ve kurulum sonrası repo kernelinin recovery dmesg kayıtlarında
cmd_db, msm_drm ve bazı ses/hwid modülleri için aynı module_layout sürüm
uyumsuzlukları görülüyor. İki kernel de recovery açabildiği ve Tachyon Android'i
de açabildiği için bu ortak uyarılar tek başına yeni bootloop'u açıklamaz.
MODULE_FORCE_LOAD açılarak uyumsuz modüller zorlanmamalıdır.

## Yalnız tanı sürümünde kayıt

Workflow'daki `boot_log=true` seçeneği `CONFIG_LISA_BOOTLOG=y` ve
`-lisa-bootdiag` sürümünü seçer. Paketleyici yalnız bu seçenek için boot
cmdline'a `lisa_bootlog=1` ekler. Hem derleme seçeneği hem cmdline gerekir;
varsayılan derleme kayıt yazmaz.

Kaydedici GPT hazır olunca blok katmanından çalışır; ADB, logd, dosya sistemi
bağlama veya `/dev` düğümlerinin hazır olmasına ihtiyaç duymaz. Yazmadan önce
cihazın tam DT modelini, sda17'nin GPT etiketinin `oops` olduğunu ve bölümün
16 MiB olduğunu kontrol eder. Eşleşmeyen bir hedefe yazmaz; normal veri,
boot, vendor veya sistem bölümleri hedeflenmez. **Bu tanı sürümü ayrılmış
oops bölümündeki eski hata kayıtlarını üzerine yazar.** Bölümün mevcut
16 MiB kopyası kurulum incelemesinde yerel olarak saklandı.

İlk 30 saniyede 500 ms, sonra 2 saniye arayla kernel ring buffer'ın son
1 MiB'sini kaydeder; 90 saniye sonra durur. Normal reboot bildirimi sırasında
son bir kayıt dener. Panic sırasında blok I/O yapmaz: ani panic/donanım reseti
için son aralıkta oluşan mesajların kaybolması mümkündür. Her açılışta iki
alternatif snapshot, toplam dört açılış tutulur. Recovery açılışı ayrı bir
nesil oluşturur; önceki açılışı hemen silmez. CRC'ler yarım yazılmış kayıtları
ayırt eder. Çalışma zamanı kaydı henüz cihaz üzerinde doğrulanmadı.

Başlık alanı ve blok I/O 4 KiB hizalıdır; UFS'nin 4 KiB mantıksal sektörünü
destekler. Daha büyük mantıksal sektörlü bir hedefe yazmayı reddeder.

Recovery'de `/dev/block/by-name/oops` bölümünü `dd ... status=none` ile
salt okunur bilgisayara alın; kopyanın tam 16777216 bayt olduğunu doğrulayın.
Yerel kopyayı şu araçla çözümleyin:

```text
python .lisa/decode_bootlog.py oops-partition.bin private-output
```

Ham kayıtlar özel bilgi içerebilir; private yerel alanda tutulur. Kernel
panic, init fatal hata, kritik servis veya modül yükleme hatası ancak ilgili
kernel sürümü ve açılış nesliyle eşleşen yeni kayıttan değerlendirilmelidir.

## Derleme ve kurulum sınırı

4 Ekim 2026 tanı derlemesi başarılı:
https://github.com/ylmzer/lisa-kernel/actions/runs/37155115623
Kaynak `c18e4724dcde865c83e07c3f93e4c9ee39eef790`, kernel
`5.4.302-lisa-bootdiag+`. Tüm CI sağlama toplamları, ARM64 Image/ELF,
etkin kaydedici, son kernel sembolleri, ZIP/Image eşleşmesi ve cmdline yamasının
tek boot yazımından önce uygulanması yerelde kontrol edildi. ZIP SHA-256:
`4bb650363f697a7872b9f7a2ae91eb89c30ec136483b898b8ffa0066872e855c`.
Paket CI sonrasında değiştirilmedi. Cihaz bağlantısı kesik olduğundan kurulum,
Android açılışı ve gerçek kayıt yazımı henüz test edilmedi. İlk tanı çalışması
37154298029, UFS blok hizalaması güncellenirken cihazda kullanılmadan iptal edildi.

Bu değişiklik kayıp logları elde etmek içindir. Kernelin Android uyumluluğunu
düzelttiği iddia edilmez. Önce CI derlemesi, kayıt çözümleyicisi ve ZIP/cmdline
kontrolleri tamamlanmalıdır. İlk iki ZIP gibi otomatik yükleme yapılmaz.
Eski kayıt yazmayan boot'a dönüş cmdline seçeneğini de kaldırır; derleme
seçeneği olmayan bir Image'de cmdline tek başına kaydı etkinleştiremez.
