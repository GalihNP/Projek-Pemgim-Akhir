# Projek_Akhir

## Animasi karakter

Sprite sheet di folder `Asset` dipakai sebagai berikut (data frame ada di `Projek_Akhir/data/animations.cpp`):

| Senjata (tombol) | Idle                  | Jalan (8 fps)          | Serangan (klik kiri / J)                                   |
|------------------|-----------------------|------------------------|------------------------------------------------------------|
| Sword (1)        | `player_weapon_1.png` | `Pemgim1.png` 3 frame  | kuda-kuda, ayunan, tebasan + efek biru (0.26 s)            |
| Pistol (2)       | `player_weapon_2.png` | `Pemgim2.png` 4 frame  | bidik, tembak + muzzle flash, recoil (0.24 s)               |
| Keris (3)        | `player_weapon_3.png` | `Pemgim3.png` 4 frame  | genggam, tarik, terjangan tusuk + efek sabetan (0.27 s)    |

- Damage diberikan tepat saat frame "hit" tampil (tebasan penuh, muzzle flash, terjangan).
- Arah dan senjata dikunci saat serangan dimulai, sehingga visual dan damage selalu cocok.
- Setelah animasi selesai karakter kembali ke pose idle (atau animasi jalan jika sedang bergerak).
- Peluru pistol digambar secara prosedural (jejak dari moncong ke arah bidikan) karena tidak ada aset peluru.
- Untuk mengubah timing, ubah kolom `duration` pada frame serangan; durasi total harus tetap
  lebih pendek dari cooldown senjata minimum (`cooldown * SkillCooldownMul(100)`).
