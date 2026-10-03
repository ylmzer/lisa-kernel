# lisa-kernel

Xiaomi 11 Lite 5G NE (`lisa` / SM7325) için bağımsız Linux 5.4.302 kaynak reposu.

**Başlangıç durumu:** Tam kaynaklar yüklendi ve GitHub ayar denetimi başarılı.
Chromium sorunu henüz çözülmüş veya cihazda doğrulanmış değildir.

Kaynak geçmişi, cihazdan alınan referans ayarlar, GitHub Actions derlemesi ve
test planı için [Lisa proje belgesine](.lisa/README.md) bakın.

İlk tanı sürümünde KernelSU/ReSukiSU bulunmaz. Derleme bu repodaki kaynaklardan
yapılır. Mevcut ROM'un DTB/DTBO dosyaları otomatik değiştirilmez.

Linux kernel belgeleri: [README](README), [COPYING](COPYING).

Mevcut ROM'un kullandığı `lz4kd` ve bazı Xiaomi ekleri bu temelde bulunmuyor.
[Doğrulanmış uyumluluk farklarını](.lisa/compatibility.md) ilk cihaz testinden
önce inceleyin.
