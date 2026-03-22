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
            octree.face.push_back(v1);
            octree.face.push_back(v2);
            octree.face.push_back(v3);
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
    
    // Parse object file
    parseFile(objFilePath, octree);

    auto start = chrono::high_resolution_clock::now();

    octree.buildOctree(optimized);
    octree.writeObj(outputPath);

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    std::cout << "Waktu eksekusi: " << duration.count() << " ms\n";

    return 0;
}
