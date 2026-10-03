# lisa-kernel

Xiaomi 11 Lite 5G NE (`lisa` / SM7325) için bağımsız Linux 5.4.302 kaynak reposu.

**Derleme durumu:** Tam kernel derlemesi ve AnyKernel3 ZIP üretimi başarılı.
İndirilen çıktılar için SHA-256, ARM64 biçimi, temel ayarlar, son kerneldeki
ekran bildirim sembolleri ve ZIP/Image eşleşmesi doğrulandı.
Chromium sorunu henüz çözülmüş veya cihazda doğrulanmış değildir.

- [İlk başarılı tam derleme](https://github.com/ylmzer/lisa-kernel/actions/runs/37129546878)
- Derlenen kaynak: `3a12e2b7a58f89c0f332f82978ee3de21efc7557`.
- Kernel sürümü: `5.4.302-lisa-ylmzer+`.
- ZIP SHA-256: `43aa38bb4b1bae00f378e27bf03933dbe07cdd4687fb8ddd7eb6ad1cc69f9a66`.

Kaynak geçmişi, cihazdan alınan referans ayarlar, GitHub Actions derlemesi ve
test planı için [Lisa proje belgesine](.lisa/README.md) bakın.

İlk tanı sürümünde KernelSU/ReSukiSU bulunmaz. Derleme bu repodaki kaynaklardan
yapılır. Mevcut ROM'un DTB/DTBO dosyaları otomatik değiştirilmez.

Linux kernel belgeleri: [README](README), [COPYING](COPYING).

Mevcut ROM'un kullandığı `lz4kd` için açık kaynak desteği eklendi; sayfa
sıkıştırma/açma testleri ve tam derleme geçti. Bazı Xiaomi ekleri bu temelde bulunmuyor.
[Uyumluluk farklarını](.lisa/compatibility.md) ilk cihaz testinden önce inceleyin.
