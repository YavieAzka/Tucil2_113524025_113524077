#include "octree.hpp"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

#ifdef _OPENMP
#include <omp.h>
#endif

using namespace std;

void Octree::showVertex() const {
    for (const auto& p : vertex)
        cout << "v " << p.x << " " << p.y << " " << p.z << "\n";
}

void Octree::showFaces() const {
    for (int i = 0; i < (int)face.size(); i += 3)
        cout << "f " << face[i]+1 << " " << face[i+1]+1 << " " << face[i+2]+1 << "\n";
}

static bool triangleAABBIntersect(
    float tx0, float ty0, float tz0,
    float tx1, float ty1, float tz1,
    float tx2, float ty2, float tz2,
    float h)
{
    // ---- 3 AABB face-normal axes (x, y, z) ----
    if (min({tx0,tx1,tx2}) >  h || max({tx0,tx1,tx2}) < -h) return false;
    if (min({ty0,ty1,ty2}) >  h || max({ty0,ty1,ty2}) < -h) return false;
    if (min({tz0,tz1,tz2}) >  h || max({tz0,tz1,tz2}) < -h) return false;

    // ---- Triangle edge vectors ----
    float e0x = tx1-tx0, e0y = ty1-ty0, e0z = tz1-tz0;
    float e1x = tx2-tx1, e1y = ty2-ty1, e1z = tz2-tz1;
    float e2x = tx0-tx2, e2y = ty0-ty2, e2z = tz0-tz2;

    // ---- Triangle normal axis ----
    float nx = e0y*e1z - e0z*e1y;
    float ny = e0z*e1x - e0x*e1z;
    float nz = e0x*e1y - e0y*e1x;
    float d  = nx*tx0 + ny*ty0 + nz*tz0;
    float r  = h * (fabs(nx) + fabs(ny) + fabs(nz));
    if (fabs(d) > r) return false;

    auto test = [&](float ax, float ay, float az) -> bool {
        float p0  = ax*tx0 + ay*ty0 + az*tz0;
        float p1  = ax*tx1 + ay*ty1 + az*tz1;
        float p2  = ax*tx2 + ay*ty2 + az*tz2;
        float rad = h * (fabs(ax) + fabs(ay) + fabs(az));
        return min({p0,p1,p2}) <= rad && max({p0,p1,p2}) >= -rad;
    };

    if (!test(   0,  e0z, -e0y)) return false;
    if (!test(-e0z,    0,  e0x)) return false;
    if (!test( e0y, -e0x,    0)) return false;
    if (!test(   0,  e1z, -e1y)) return false;
    if (!test(-e1z,    0,  e1x)) return false;
    if (!test( e1y, -e1x,    0)) return false;
    if (!test(   0,  e2z, -e2y)) return false;
    if (!test(-e2z,    0,  e2x)) return false;
    if (!test( e2y, -e2x,    0)) return false;

    return true;   
}

void Octree::voxelize(OctreeNode* node, int depth, bool optimized) {
    Point  center = node->getCenter();
    float  size   = node->getSize();
    float  half   = size * 0.5f;

    vector<int> hitting;
    hitting.reserve(node->faceIndices.size());

    for (int idx : node->faceIndices) {
        int base = idx * 3;
        const Point& v0 = vertex[face[base]];
        const Point& v1 = vertex[face[base+1]];
        const Point& v2 = vertex[face[base+2]];

        if (triangleAABBIntersect(
                v0.x - center.x, v0.y - center.y, v0.z - center.z,
                v1.x - center.x, v1.y - center.y, v1.z - center.z,
                v2.x - center.x, v2.y - center.y, v2.z - center.z,
                half))
        {
            hitting.push_back(idx);
        }
    }

    { vector<int>().swap(node->faceIndices); }

    if (hitting.empty()) {
        if (depth > 0) {
#pragma omp atomic
            prunedNodes[depth]++;
        }
        return;
    }

    if (depth > 0) {
#pragma omp atomic
        nodeCountPerDepth[depth]++;
    }

    // ------ BASE CASE: leaf voxel ------
    if (depth == maxDepth) {
        node->isLeaf  = true;
        node->isVoxel = true;
        return;
    }

    // ------ DIVIDE: create 8 child octants ------
    node->isLeaf = false;
    float childSize = size * 0.5f;
    float off       = childSize * 0.5f;

    int ci = 0;
    for (int dz = -1; dz <= 1; dz += 2)
        for (int dy = -1; dy <= 1; dy += 2)
            for (int dx = -1; dx <= 1; dx += 2) {
                OctreeNode* child = new OctreeNode();
                Point cc = { center.x + dx*off,
                             center.y + dy*off,
                             center.z + dz*off };
                child->setCenter(cc);
                child->setSize(childSize);
                child->faceIndices = hitting;  
                node->children[ci++] = child;
            }

    if (optimized && depth == 0) {
#pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < 8; i++)
            voxelize(node->children[i], depth + 1, optimized);
    } else {
        for (int i = 0; i < 8; i++)
            voxelize(node->children[i], depth + 1, optimized);
    }
}

void Octree::buildOctree(bool optimized) {
    if (vertex.empty() || face.empty()) return;

    // ---- Compute axis-aligned bounding box ----
    float minX = vertex[0].x, maxX = vertex[0].x;
    float minY = vertex[0].y, maxY = vertex[0].y;
    float minZ = vertex[0].z, maxZ = vertex[0].z;

    for (const auto& p : vertex) {
        minX = min(minX, p.x);  maxX = max(maxX, p.x);
        minY = min(minY, p.y);  maxY = max(maxY, p.y);
        minZ = min(minZ, p.z);  maxZ = max(maxZ, p.z);
    }

    float cx   = (minX + maxX) * 0.5f;
    float cy   = (minY + maxY) * 0.5f;
    float cz   = (minZ + maxZ) * 0.5f;
    float span = max({ maxX-minX, maxY-minY, maxZ-minZ });
    float size = span * 1.001f;  

    // ---- Build root ----
    root = new OctreeNode();
    root->setCenter({cx, cy, cz});
    root->setSize(size);

    int numFaces = (int)face.size() / 3;
    root->faceIndices.resize(numFaces);
    for (int i = 0; i < numFaces; i++) root->faceIndices[i] = i;

    voxelize(root, 0, optimized);
}

void Octree::collectVoxels(OctreeNode* node, ofstream& file, int& offset) {
    if (!node) return;

    if (node->isLeaf && node->isVoxel) {
        Point c = node->getCenter();
        float h = node->getSize() * 0.5f;

        // ---- 8 vertices ----
        file << "v " << (c.x-h) << " " << (c.y-h) << " " << (c.z-h) << "\n"; // 1
        file << "v " << (c.x+h) << " " << (c.y-h) << " " << (c.z-h) << "\n"; // 2
        file << "v " << (c.x+h) << " " << (c.y+h) << " " << (c.z-h) << "\n"; // 3
        file << "v " << (c.x-h) << " " << (c.y+h) << " " << (c.z-h) << "\n"; // 4
        file << "v " << (c.x-h) << " " << (c.y-h) << " " << (c.z+h) << "\n"; // 5
        file << "v " << (c.x+h) << " " << (c.y-h) << " " << (c.z+h) << "\n"; // 6
        file << "v " << (c.x+h) << " " << (c.y+h) << " " << (c.z+h) << "\n"; // 7
        file << "v " << (c.x-h) << " " << (c.y+h) << " " << (c.z+h) << "\n"; // 8

        int b = offset;   // 1-based start index

        // ---- 12 triangular faces (outward CCW normals) ----
        // Front  (z−): normal (0,0,−1)
        file << "f " << b+0 << " " << b+3 << " " << b+2 << "\n";
        file << "f " << b+0 << " " << b+2 << " " << b+1 << "\n";
        // Back   (z+): normal (0,0,+1)
        file << "f " << b+4 << " " << b+5 << " " << b+6 << "\n";
        file << "f " << b+4 << " " << b+6 << " " << b+7 << "\n";
        // Left   (x−): normal (−1,0,0)
        file << "f " << b+0 << " " << b+4 << " " << b+7 << "\n";
        file << "f " << b+0 << " " << b+7 << " " << b+3 << "\n";
        // Right  (x+): normal (+1,0,0)
        file << "f " << b+1 << " " << b+2 << " " << b+6 << "\n";
        file << "f " << b+1 << " " << b+6 << " " << b+5 << "\n";
        // Bottom (y−): normal (0,−1,0)
        file << "f " << b+0 << " " << b+1 << " " << b+5 << "\n";
        file << "f " << b+0 << " " << b+5 << " " << b+4 << "\n";
        // Top    (y+): normal (0,+1,0)
        file << "f " << b+3 << " " << b+6 << " " << b+2 << "\n";
        file << "f " << b+3 << " " << b+7 << " " << b+6 << "\n";

        offset += 8;
        return;
    }

    for (int i = 0; i < 8; i++)
        collectVoxels(node->children[i], file, offset);
}

void Octree::writeObj(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Tidak dapat membuka file output: " << filename << "\n";
        return;
    }

    file << "# Voxelized OBJ — generated by Octree voxelizer\n";
    file << "# Uniform voxel size at depth " << maxDepth << "\n\n";

    int offset = 1;   // OBJ indices are 1-based
    collectVoxels(root, file, offset);
}