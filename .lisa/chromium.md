# Chromium sorunu: başlangıç kanıtı ve test planı

## Gözlenen durum

Önceki Chrome/Edge kayıtlarında native `zygote-child` süreçleri SIGABRT ile
sonlanıyor. Daha sonra tarayıcı GPU kanalını beklerken zaman aşımıyla kapanıyor.
Bu sıra, GPU hata mesajının süreç başlatma sorununun sonucu olabileceğini
gösterir; belirli bir GPU sürücüsü hatasını kanıtlamaz.

Bazı `capset` ve ambient capability hataları çökme kaydı toplayan çocuk süreç
başladıktan sonra görülüyor. Bunları ilk SIGABRT'nin nedeni olarak yorumlamak
için yeterli kanıt yoktur. `SetClamps` uyarıları ve haptics `invalid GPIO -1`
uyarıları için de tarayıcıyla doğrudan nedensellik gösterilmedi.

Tachyon'da KernelSU manual/auto-setuid kancaları etkin. Bu repodaki ilk tanı
kernelinde bu kancalar bulunmaz. Kaynak ağacı ve derleyici de değiştiği için
sonuçtaki davranış farkı tek başına KernelSU'ya bağlanamaz.

## 3 Ekim 2026 canlı incelemesi

Çalışan Tachyon 5.4.302 üzerinde Chrome 154.0.8037.94 ana süreç tombstone'u
`Timed out waiting for GPU channel.` hatasını doğruladı. Ayrı canlı kayıtlarda
native `zygote-child` SIGABRT ve NativeOnlySandboxedProcessService bağlantı
sorunları gözlendi. Kullanılabilir çocuk backtrace'i alınamadı; belirli bir
syscall veya KernelSU kaynaklı hata henüz kanıtlanmadı.

Geçici `--disable-features=JavalessRendererExperimentOn` denemesinde Chrome'un
komut dosyasını okuduğu doğrulandı; native çocuk çökmesi tekrarlandı. Java
süreç yoluna geçiş ve test sayfasının yüklenmesi doğrulanamadığı için deney
sonuçsuzdur. Debug-app ayarı ve geçici dosya temizlenip kontrol edildi.
Ham loglar, tombstone'lar ve APK private yerel klasörde tutulur.

**İlk derlenen tanı kerneli cihazda açılmadı.** Kernel karşılaştırması yapılmış
sayılmaz. Aşağıdaki kernel adımları açılış sorunu çözülmeden uygulanmamalıdır.

## Kontrollü karşılaştırma

1. Aynı ROM ve aynı tarayıcı sürümüyle başlangıç kayıtlarını saklayın.
2. Kernel testi için kullanılan ZIP, `SHA256SUMS` ve `build-info.txt` dosyalarını
   birlikte saklayın. Çalışan boot'un geri dönüş yedeği ayrıca hazırlanmalıdır.
3. Test kernelinde önce açılış, ekran, dokunma, ağ, ses, kamera ve uyku/uyanmayı
   doğrulayın. ZRAM'da `lz4kd` seçildiğini ve `/proc/swaps` alanını kontrol edin.
4. Tarayıcı açılışını, bir sayfa yüklemesini ve video oynatmayı aynı koşullarda
   deneyin. Başlangıç ve test kernelinin kayıtlarını `capture-device.ps1` ile
   farklı klasörlerde saklayın.
5. SIGABRT devam ederse native zygote tombstone'unu ve `zygote_next` /
   `libzygote_sys` kullanıcı alanı yolunu inceleyin. Kaynakta başlangıç noktaları
   `kernel/fork.c`, `kernel/sys.c` ve `security/commoncap.c` dosyalarıdır;
   bu dosyalar şu anda kanıtlanmış hata veya yama noktaları değildir.

Kernel ayarlarını rastgele açmak, GPU kanal zaman aşımını bastırmak veya
modülleri zorla yüklemek doğrulanmış bir Chromium düzeltmesi değildir.


## 5 Ekim 2026: çalışan dd06b185 ile güncel kullanıcı denemesi

5G/HIDL çalışması kullanıcı isteğiyle bilgisayar bağlantısı Wi-Fi'ye geçilene
kadar ertelendi. Mobil bağlantıya müdahale edilmeden ADB üzerinden mevcut
kernel/logcat, DropBox ANR/çökme, WebView ve süreç kayıtları alındı. Kullanıcı
Edge, Chrome, BonusFlaş ve ÖSYM GİS uygulamalarını örnek verdi ve uygulamaları
kendisi açarak denedi. Ajan uygulama başlatmadı, durdurmadı veya telefonu yeniden
başlatmadı; ROM/kernel dosyalarını değiştirmedi.

Mevcut boot kimliği daha önce doğrulanan dd06b185 ile aynı. Güncel WebView
153.0.8010.36 etkin; her iki RELRO tamamlanmış. Bu, tüm WebView işlemlerinin
başarılı olduğunu tek başına kanıtlamaz.

08:51:23 ve 08:52:31'de Chrome'un NativeOnlySandboxedProcessService başlatma
yolundaki native zygote-child SIGABRT ile çöküyor. System_server zygote soketine
bağlanırken Connection refused alıyor. Çökme toplayıcısının capset/ambient
capability hataları ilk SIGABRT'den sonra geliyor; bunları ilk çöküşün nedeni
olarak kabul etmek doğru değildir. Çocuk backtrace'i hâlâ elde edilemedi.

Chrome ana sürecinin kendi iş parçacığı ayrıca mevcut olmayan
android.hardware.drm.IDrmFactory/widevine AIDL servisini tekrar tekrar bekliyor.
Önceki Edge/Chrome tombstone'larında GPU kanalı zaman aşımı bulunuyor; güncel
iki çocuk SIGABRT'sini belirli bir GPU kernel hatasına bağlayan backtrace yok.

08:53:55 BonusFlaş 4.3.1 için yeni startup ANR kaydı mevcut. Ana iş parçacığı
BonusApplication.onCreate / createHashedSignatureAndDeviceSerialId üzerinden
MediaDrm oluştururken libmediadrm makeDrmFactoriesAidlForUuid,
AServiceManager_waitForService ve futex/condition wait içinde bekliyor. Eski
23:38 ve 00:01 ANR'leri de aynı bekleme yolunu gösteriyor. Bu uygulamanın
başlangıç kilitlenmesi için doğrudan bir servis bekleme kanıtıdır.

Cihazda HIDL DRM 1.3 clearkey/widevine süreçleri ve kayıtlı HIDL arayüzleri
bulunuyor; sorgulanan Binder servis listesinde Widevine AIDL sağlayıcısı yok.
Logcat, AIDL Widevine servisinin başlatılamadığını gösteriyor. HIDL sağlayıcısının
varlığı her UUID/plugin işleminin çalıştığını doğrulamaz. ROM'un AIDL bekleme /
HIDL fallback davranışı ayrıca incelenmelidir; sahte DRM sonucu veya güvenlik
kontrolü atlatma önerilmemiştir.

ÖSYM GİS ve Edge için bu yeni zaman diliminde eşdeğer nedensel stack henüz
çıkarılmadı. Bütün uygulamaların aynı kök nedenden etkilendiği söylenemez.

Elde edilen güncel kernel halkasında KGSL GPU fault, kernel panic/hung-task
veya OOM-kill ile bu olaylar arasında nedensel eşleşme bulunmadı. Halka geçmişin
tamamını kapsamıyor. KGSL reset_count normal GPU başlatmalarında da artar;
sayacın yüksek olması tek başına GPU çöküş sayısı değildir.

Öncelik: native zygote'ın ilk abort/backtrace'ini belirlemek ve gerçek DRM
sağlayıcısı seçiminin neden eksik AIDL servisini beklediğini doğrulamak. Bu
bulgularla kanıtlanmış bir kernel düzeltmesi veya yükleme paketi henüz yoktur.
Ham uygulama kayıtları ve ANR içerikleri private/ignored klasörde saklanır.


## Hazırlanan ayrı test adımları

DRM için bilgisayarda 32/64-bit libmediadrm test adayları hazırlandı. CRB
orijinalleri cihazdaki SHA-256 değerleriyle eşleşti. Undefined import
AServiceManager_waitForService yerine AServiceManager_checkService seçiliyor;
kayıtlı AIDL sağlayıcısı varsa gerçek Binder alınır, yoksa mevcut null/error
ve HIDL geri dönüşü çalışabilir. Kütüphanenin kodu, exported ABI, relocation
ve GNU hash bölümleri aynı. Her ABI'de 14 byte değişir. DRM UUID/key/license,
TEE, güvenlik kontrolü veya başarılı sonuç taklidi yapılmaz.

Aday cihaza uygulanmadı. Mevcut HIDL plugin'in uygulamalar tarafından başarıyla
kullanılması ayrıca test edilmelidir. Sadece AIDL kullanan, geç başlayan lazy
servislerde lookup erken başarısız olabilir; bu aday mevcut HIDL Lisa ROM'una
özgü test içindir. Orijinaller ve manifest ayrı private aday paketinde saklanır.

Chrome için opt-in CONFIG_LISA_ZYGOTE_DIAG eklendi. Native zygote-child'ın
seçilmiş başlangıç syscall numarası ve sonuçları en fazla 256 kez/boot kaydedilir.
Compat süreçler ve benzer ama farklı isimler dışlanır; syscall argümanları
kaydedilmez. Invoke sonrası ve seccomp/ptrace tarafından kısa kesilen yol
izlenir. Syscall sonuçları ve güvenlik kararları değişmez. Bu bir Chromium
çözümü değil, ilk başarısız çağrıyı bulmak için geçici tanı aracıdır.

DRM yaması ile tanı kernelini aynı anda test etmek yerine önce ayrı sonuçlar
alınmalıdır. Sisteme uygulanacak dosyalar için tam hash doğrulaması, geri dönüş
ve metadata/SELinux korunması gerekir. Mobil PC bağlantısını bozacak kurulum
veya yeniden başlatma ajan tarafından yapılmadı.
