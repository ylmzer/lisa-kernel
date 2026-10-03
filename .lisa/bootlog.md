# Android açılış tanısı

İkinci cihaz testinde repo kerneli TWRP'yi başlattı, fakat Android açılışı
yeniden başlama döngüsüne girdi. Pstore boş ve last_kmsg yoktu. ROM'un cache
arşivleri eski qgki/Tachyon denemelerine ait; güncel `.partial` arşivi boştu.
Bu yüzden henüz doğrulanmış bir açılış yaması yoktur.

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

Bu değişiklik kayıp logları elde etmek içindir. Kernelin Android uyumluluğunu
düzelttiği iddia edilmez. Önce CI derlemesi, kayıt çözümleyicisi ve ZIP/cmdline
kontrolleri tamamlanmalıdır. İlk iki ZIP gibi otomatik yükleme yapılmaz.
Eski kayıt yazmayan boot'a dönüş cmdline seçeneğini de kaldırır; derleme
seçeneği olmayan bir Image'de cmdline tek başına kaydı etkinleştiremez.
