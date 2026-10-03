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
