#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>
#include <string>
#include "octree.hpp"

using namespace std;




void parseFile(const string& path, Octree& octree) {
    ifstream file(path);
    string line;
    while (getline(file, line)) {
        istringstream ss(line);
        string token;
        ss >> token;  

        if(token == "v") {
            Point p;
            ss >> p.x >> p.y >> p.z;
            octree.vertex.push_back(p);
        } else if (token == "f") {
            int v1, v2, v3;
            ss >> v1 >> v2 >> v3;
            octree.face.push_back(v1 - 1);
            octree.face.push_back(v2 - 1);
            octree.face.push_back(v3 - 1);
        }

       
    }
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Cara Penggunaan: " << argv[0] << " <path_file.obj> <max_depth> <output_file.obj> <1(parallel)/0(not parallel)>\n";
        return 1; // Return non-zero menandakan program keluar karena error
    }

    string objFilePath = argv[1];
    int maxDepth;
    string outputPath = argv[3];
    bool optimized = (string(argv[4]) == "1");

    // Validasi input kedalaman 
    try {
        maxDepth = atoi(argv[2]);
        if (maxDepth <= 0 && string(argv[2]) != "0") {
            cerr << "Error: Kedalaman maksimum harus berupa angka bulat positif.\n";
            return 1;
        }
    } catch (const exception& e) {
        cerr << "Error: Kedalaman maksimum harus berupa angka bulat (integer).\n";
        return 1;
    }

    // Validasi apakah file eksis dan bisa dibuka
    ifstream file(objFilePath);
    if (!file.is_open()) {
        cerr << "Error: Tidak dapat membuka atau menemukan file di path: " << objFilePath << "\n";
        return 1;
    }
    file.close(); 

    std::cout << "Input diterima dengan baik!\n";
    std::cout << "File OBJ target : " << objFilePath << "\n";
    std::cout << "Kedalaman Octree: " << maxDepth << "\n";
    std::cout << "------------------------------------\n";

    Octree octree;
    octree.maxDepth = maxDepth;
    for(int i = 0; i <= maxDepth; i++) {
        octree.nodeCountPerDepth.push_back(0);
    }
    for(int i = 0; i <= maxDepth; i++) {
        octree.prunedNodes.push_back(0);
    }
    //octree.nodeCountPerDepth[maxDepth] = 1; // Root node dihitung sebagai 1 pada kedalaman maksimum
    
    // Parse object file
    parseFile(objFilePath, octree);

    auto start = chrono::high_resolution_clock::now();

    octree.buildOctree(optimized);
    octree.writeObj(outputPath);

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Waktu eksekusi: " << duration.count() << " ms\n";

    cout << "STATISTIK:\n";
    cout << "Banyaknya voxel yang terbentuk : " << octree.nodeCountPerDepth[maxDepth] << "\n";
    cout << "Banyaknya vertex yang terbentuk: " << octree.nodeCountPerDepth[maxDepth] * 8 << "\n";
    cout << "Banyaknya faces yang terbentuk : " << octree.nodeCountPerDepth[maxDepth] * 12 << "\n";
    
    cout << "Node octree yang terbentuk: \n";
    for(int i = 1; i <= octree.maxDepth; i++) {
        cout << i << " : " << octree.nodeCountPerDepth[i] << "\n";
    }
    
    cout << "Node yang tidak perlu ditelusuri:\n";
    for(int i = 1; i <= octree.maxDepth; i++) {
        cout << i << " : " << octree.prunedNodes[i] << "\n";
    }

    cout << "Kedalaman octree: " << octree.maxDepth << "\n";
    cout << "Waktu perhitungan: " << duration.count() << " ms\n";
    cout << "Path hasil .obj disimpan: " << outputPath << "\n";
    
    return 0;
}
