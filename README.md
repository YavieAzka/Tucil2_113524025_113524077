# Tugas Kecil 2 IF2211 Strategi Algoritma: Voxelization Objek 3D menggunakan Octree

## Penjelasan Singkat Program

Program ini adalah aplikasi berbasis _Command Line Interface_ (CLI) yang ditulis dalam bahasa C++ untuk mengonversi model 3D standar (berformat `.obj`) menjadi model voxel. Proses transformasi ini mengimplementasikan algoritma _Divide and Conquer_ dengan memanfaatkan struktur data pohon _Octree_.

Program bekerja dengan mencari kotak pembatas (_bounding box_) dari model 3D, lalu membaginya menjadi 8 sub-oktan secara rekursif hingga mencapai batas kedalaman maksimum yang ditentukan pengguna. Setiap sub-oktan akan diuji persinggungannya dengan permukaan objek menggunakan metode _Separating Axis Theorem_ (SAT). Sub-oktan yang kosong akan dipangkas (_pruned_), sementara yang bersinggungan akan terus dibagi hingga menjadi voxel pada kedalaman maksimum. Program ini juga dilengkapi dengan fitur konkurensi (paralelisasi) menggunakan OpenMP untuk mempercepat proses komputasi pada kedalaman pohon yang tinggi.

## Requirement Program

Untuk melakukan kompilasi dan menjalankan program ini, diperlukan:

1. **Sistem Operasi:** Windows atau Linux.
2. **Compiler C++:** `g++` (mendukung standar C++11 atau lebih baru).
3. **OpenMP:** Pustaka OpenMP (biasanya sudah terintegrasi dengan `gcc`/`g++` modern) untuk menjalankan fitur konkurensi (Spesifikasi Bonus).

## Cara Mengkompilasi Program

Buka terminal atau _command prompt_, lalu navigasikan ke _root directory_ repositori ini. Jalankan perintah berikut untuk mengkompilasi _source code_ yang berada di dalam folder `src` dan menyimpan _executable_ ke dalam folder `bin`:

**Untuk Linux/Windows (dengan g++):**

```bash
g++ -fopenmp src/main.cpp src/octree.cpp -o bin/voxelizer
```

_(Catatan: `-fopenmp` wajib disertakan agar fitur konkurensi berfungsi dengan baik. Kompilasi ini bersifat opsional karena hasil compile sudah terdapat pada bin/voxelizer.exe)._

## Cara Menjalankan dan Menggunakan Program

Setelah berhasil dikompilasi, Anda dapat menjalankan program dari _root directory_ menggunakan format argumen berikut:

```bash
./bin/voxelizer <path_file.obj> <max_depth> <output_file.obj> <1/0>
```

**Penjelasan Argumen:**

- `<path_file.obj>` : Lokasi file input model 3D yang ingin dikonversi.
- `<max_depth>` : Kedalaman maksimum pohon Octree (berupa angka bulat positif).
- `<output_file.obj>` : Lokasi dan nama file tempat hasil konversi (voxel) akan disimpan.
- `<1/0>` : _Flag_ untuk fitur optimasi konkurensi. Ketik `1` untuk menggunakan metode paralel (OpenMP), atau `0` untuk eksekusi sekuensial biasa.

**Contoh Penggunaan:**

```bash
./bin/voxelizer test/pumpkin.obj 6 test/pumpkin-voxelized.obj 1
```

Setelah dijalankan, program akan menampilkan log validasi, statistik pembuatan node per kedalaman, jumlah voxel/vertex/faces yang terbentuk, serta total waktu eksekusi.

## Author / Identitas Pembuat

Tugas ini dikerjakan oleh:

- **Yavie Azka Putra Araly** - 13524077
- **Moh. Hafizh Irham Perdana** - 13524025

Dibuat untuk memenuhi Tugas Kecil 2 IF2211 Strategi Algoritma, Semester II tahun 2025/2026, Institut Teknologi Bandung.
