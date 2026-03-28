#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
using namespace std;

struct Point {
    float x;
    float y;
    float z;
};

class OctreeNode {
    private:
        Point center;
        float size;
        
    public:
        bool isLeaf;
        bool isVoxel;
        OctreeNode* children[8];
        vector<int> faceIndices;  
        OctreeNode() : isLeaf(true), isVoxel(false) {
            for (int i = 0; i < 8; i++) children[i] = nullptr;
        }

        ~OctreeNode() {
            for (int i = 0; i < 8; i++)
                delete children[i]; 
        }

        float getSize() const {
            return size;
        }

        void setSize(float newSize) {
            size = newSize;
        }

        Point getCenter() const {
            return center;
        }

        void setCenter(const Point& newCenter) {
            center = newCenter;
        }

        
};

class Octree {
    private:
        OctreeNode* root;
        
    public:
        vector <Point> vertex;
        int maxDepth;
        vector <int> nodeCountPerDepth;
        vector <int> face;
        vector <int> prunedNodes;
        Octree() : root(nullptr), maxDepth(0) {}

        ~Octree() {
            delete root; 
        }

        void showVertex() const;
        void showFaces() const;

        void voxelize(OctreeNode* node, int depth, bool optimized);

        void buildOctree(bool optimized = false);
        void writeObj(const string& filename);
        void collectVoxels(OctreeNode* node, ofstream& file, int& offset);
};