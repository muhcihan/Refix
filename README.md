# 🤖 Refix Robot Yüz

ESP32-C3 Super Mini ve 240x240 renkli ekranla yapılmış, **göz kırpan, uyuyan, uyanan, sallanınca başı dönen** sevimli bir robot yüzü. Tek bir Arduino kodu, birkaç kablo ve bir LiPo pil yeterli. Lehim gerekmiyor.

> Bu proje [Refix](https://instagram.com/muhcihan) için hazırlandı: mühendisliği çocuklara, ailelere ve meraklılara anlaşılır kılmak için.

![image](https://github.com/muhcihan/Refix/issues/1)

---

## ✨ Ne yapıyor?

Robot, kod içindeki bir senaryoyla kendi kendine oynar ve başa sarar:

| Süre | Ne oluyor |
|---|---|
| 0–2,5 sn | Uyuyor, yukarı süzülen **Zzz** harfleri |
| 2,5 sn | Uyanır, mutlu gözlerle **"Günaydın!"** der |
| 4,3 sn | **"Merhaba, ben robot!"** |
| 6,7–9 sn | **"Etrafa bakayım..."** diyerek gözleri sola, sonra sağa kayar |
| 9,6–12,9 sn | Gözler spirale döner, yıldızlar uçuşur: **"Yapma, başım döndü!"** |
| 12,9 sn | **"Hh... geçti galiba."** |
| 15,3–17,3 sn | **"Uykum geldi..."**, gözler kapanır |
| 22,3 sn | Sabit **Refix** logo ekranı |
| 28 sn | Başa sarar |

Öne çıkan detaylar:

- 💬 Alt yazılar **daktilo efektiyle** harf harf yazılır
- 🇹🇷 **Türkçe karakterler** (ş, ğ, ı, ö, ü, ç) ek font kütüphanesi olmadan çizilir
- 🎞️ **Titreşimsiz animasyon**: tüm kare önce hafızada çizilir, sonra ekrana tek seferde basılır (~30 FPS)
- 👀 Göz kırpma, nefes alma, parıltılı gözler, yanaklar ve dalgalı ağız

---

## 🧰 Malzemeler

| Parça | Adet | Not |
|---|---|---|
| ESP32-C3 Super Mini | 1 | Ana mikrodenetleyici |
| ESP32-C3 Super Mini genişletme kartı | 1 | LiPo pil girişi ve şarj devresi olan model |
| 240x240 renkli ekran (ST7789, SPI) | 1 | IPS LCD; çoğu satıcı "OLED" yazıyor ama **LCD**'dir |
| Dişi-dişi jumper kablo | 6–7 | Lehim gerekmez |
| LiPo pil (3.7 V) | 1 | Genişletme kartının konnektörüne uygun olsun |
| USB-C kablo | 1 | Kod yüklemek ve şarj için |

> 💡 Ekran modülünde sadece **7 pin** varsa (GND, VCC, SCL, SDA, RES, DC, BLK) CS pini yoktur. Bu projenin kodu da bu tip modül için yazıldı.

---

## 🔌 Bağlantılar

Ekran, ESP32-C3'ün donanımsal SPI pinlerini kullanır.

| Ekran pini | ESP32-C3 Super Mini | Açıklama |
|---|---|---|
| **GND** | GND | Toprak |
| **VCC** | 3V3 | Besleme (3.3 V) |
| **SCL** | GPIO 4 | SPI saat (SCK) |
| **SDA** | GPIO 6 | SPI veri (MOSI) |
| **RES** | GPIO 1 | Reset |
| **DC** | GPIO 0 | Veri/komut seçimi |
| **BLK** | 3V3 | Arka ışık (her zaman açık) |
| CS *(varsa)* | GND | CS pini olan modellerde GND'ye bağla |

```text
   Ekran                 ESP32-C3 Super Mini
 ┌────────┐             ┌──────────────────┐
 │  GND   │─────────────│ GND              │
 │  VCC   │─────────────│ 3V3              │
 │  SCL   │─────────────│ GPIO 4           │
 │  SDA   │─────────────│ GPIO 6           │
 │  RES   │─────────────│ GPIO 1           │
 │  DC    │─────────────│ GPIO 0           │
 │  BLK   │─────────────│ 3V3              │
 └────────┘             └──────────────────┘
