#include "octree.hpp"


void Octree::showVertex() const {
    cout << "Vertices:\n";
    for (size_t i = 0; i < vertex.size(); i++) {
        cout << "Vertex " << i << ": (" << vertex[i].x << ", " << vertex[i].y << ", " << vertex[i].z << ")\n";
    }
}

void Octree::showFaces() const {
    cout << "Faces:\n";
    for (size_t i = 0; i < face.size(); i += 3) {
        cout << "Face " << i/3 << ": (" << face[i] << ", " << face[i+1] << ", " << face[i+2] << ")\n";
    }
}

float dotProduct(const Point& a, const Point& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Point crossProduct(const Point& a, const Point& b) {
    return Point{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// Returns true if projections of triangle and cube overlap on the given axis
bool overlapOnAxis(Point v0, Point v1, Point v2,
                   Point halfSize, Point axis) {
    // project triangle vertices onto the axis
    float p0 = dotProduct(v0, axis);
    float p1 = dotProduct(v1, axis);
    float p2 = dotProduct(v2, axis);

    // projects radius of the cube onto the axis
    float r = halfSize.x * fabs(axis.x)
            + halfSize.y * fabs(axis.y)
            + halfSize.z * fabs(axis.z);

    float triMin = min({p0, p1, p2});
    float triMax = max({p0, p1, p2});

    // overlap: if triMin <= r and triMax >= -r
    return !(triMin > r || triMax < -r);
}

// center = pusat kubus, halfSize = setengah panjang sisi
bool triangleIntersects(Point v0, Point v1, Point v2,
                             Point center, Point halfSize) {
    // translate triangle to cube's local space
    v0 = {v0.x - center.x, v0.y - center.y, v0.z - center.z};
    v1 = {v1.x - center.x, v1.y - center.y, v1.z - center.z};
    v2 = {v2.x - center.x, v2.y - center.y, v2.z - center.z};

    // triangle edges
    Point e0 = {v1.x-v0.x, v1.y-v0.y, v1.z-v0.z};
    Point e1 = {v2.x-v1.x, v2.y-v1.y, v2.z-v1.z};
    Point e2 = {v0.x-v2.x, v0.y-v2.y, v0.z-v2.z};

    Point axisX = {1,0,0}, axisY = {0,1,0}, axisZ = {0,0,1};

    // 9 axes cross produt
    Point axes9[9] = {
        crossProduct(e0, axisX), crossProduct(e0, axisY), crossProduct(e0, axisZ),
        crossProduct(e1, axisX), crossProduct(e1, axisY), crossProduct(e1, axisZ),
        crossProduct(e2, axisX), crossProduct(e2, axisY), crossProduct(e2, axisZ),
    };
    for (auto& ax : axes9)
        if (!overlapOnAxis(v0, v1, v2, halfSize, ax)) return false;

    // 3 main axes 
    if (!overlapOnAxis(v0, v1, v2, halfSize, axisX)) return false;
    if (!overlapOnAxis(v0, v1, v2, halfSize, axisY)) return false;
    if (!overlapOnAxis(v0, v1, v2, halfSize, axisZ)) return false;

    // triangle's normal
    Point normal = crossProduct(e0, e1);
    if (!overlapOnAxis(v0, v1, v2, halfSize, normal)) return false;

    return true;
}


void Octree::voxelize(OctreeNode* node, int depth, bool optimized) {
    if (depth == 0) {
        node->isLeaf = true;
        node->isVoxel = true; 
        return;
    }

    Point center = node->getCenter();
    Point offsets[8] = {
        { -node->getSize() / 4, -node->getSize() / 4, -node->getSize() / 4 }, // Octant 0   
        {  node->getSize() / 4, -node->getSize() / 4, -node->getSize() / 4 }, // Octant 1
        { -node->getSize() / 4,  node->getSize() / 4, -node->getSize() / 4 }, // Octant 2
        {  node->getSize() / 4,  node->getSize() / 4, -node->getSize() / 4 }, // Octant 3
        { -node->getSize() / 4, -node->getSize() / 4,  node->getSize() / 4 }, // Octant 4
        {  node->getSize() / 4, -node->getSize() / 4,  node->getSize() / 4 }, // Octant 5
        { -node->getSize() / 4,  node->getSize() / 4,  node->getSize() / 4 }, // Octant 6
        {  node->getSize() / 4,  node->getSize() / 4,  node->getSize() / 4 }  // Octant 7
    };

    bool anyChild = false;

    #pragma omp parallel for if(depth >= maxDepth - 2 && optimized)
    for(int i = 0; i < 8; i++) {
        node->children[i] = new OctreeNode();
            node->children[i]->setSize(node->getSize() / 2);
            Point newCenter = { center.x + offsets[i].x, center.y + offsets[i].y, center.z + offsets[i].z };
            node->children[i]->setCenter(newCenter);


            // Check if any triangle intersects with this child
            bool intersects = false; 
            for (int idx : node->faceIndices) {
                Point v0 = this->vertex[this->face[idx * 3]];
                Point v1 = this->vertex[this->face[idx * 3 + 1]];
                Point v2 = this->vertex[this->face[idx * 3 + 2]];

                if (triangleIntersects(v0, v1, v2, newCenter, Point{node->getSize() / 2, node->getSize() / 2, node->getSize() / 2})) {
                    node->children[i]->faceIndices.push_back(idx);
                    intersects = true;
                }
            }
            if(intersects) {
                anyChild = true;
                voxelize(node->children[i], depth - 1, optimized);
            } else {
                delete node->children[i];
                node->children[i] = nullptr;
            }
    }

    

    // After creating children, this node is no longer a leaf
    if(anyChild){
        node->isLeaf = false;
    }
    
}

void Octree::buildOctree(bool optimized) {
    float minX, minY, minZ, maxX, maxY, maxZ;
    minX = minY = minZ = std::numeric_limits<float>::max();
    maxX = maxY = maxZ = std::numeric_limits<float>::lowest();
    for(const auto& v : vertex) {
        minX = min(minX, v.x);
        minY = min(minY, v.y);
        minZ = min(minZ, v.z);
        maxX = max(maxX, v.x);
        maxY = max(maxY, v.y);
        maxZ = max(maxZ, v.z);
    }

    Point center;
    center.x = (minX + maxX) / 2;
    center.y = (minY + maxY) / 2;
    center.z = (minZ + maxZ) / 2;

    float size = max({maxX - minX, maxY - minY, maxZ - minZ});

    this->root = new OctreeNode();
    this->root->setCenter(center);
    this->root->setSize(size);

    int faceCount = face.size() / 3;
    for (int i = 0; i < faceCount; i++) {
        root->faceIndices.push_back(i);
    }

    voxelize(root, maxDepth, optimized);
}

void Octree::writeObj(const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "Error: tidak bisa membuka file output\n";
        return;
    }

    int vertexOffset = 1; // indeks di .obj dimulai dari 1
    collectVoxels(root, file, vertexOffset);

    file.close();
    cout << "Output disimpan di: " << filename << "\n";
}

void Octree::collectVoxels(OctreeNode* node, ofstream& file, int& offset) {
    if (node == nullptr) return;

    // Jika ini voxel aktif → tulis kubus
    if (node->isVoxel) {
        Point c = node->getCenter();
        float h = node->getSize() / 2.0f;

        // 8 vertex kubus
        file << "v " << c.x-h << " " << c.y-h << " " << c.z-h << "\n"; // v0
        file << "v " << c.x+h << " " << c.y-h << " " << c.z-h << "\n"; // v1
        file << "v " << c.x+h << " " << c.y+h << " " << c.z-h << "\n"; // v2
        file << "v " << c.x-h << " " << c.y+h << " " << c.z-h << "\n"; // v3
        file << "v " << c.x-h << " " << c.y-h << " " << c.z+h << "\n"; // v4
        file << "v " << c.x+h << " " << c.y-h << " " << c.z+h << "\n"; // v5
        file << "v " << c.x+h << " " << c.y+h << " " << c.z+h << "\n"; // v6
        file << "v " << c.x-h << " " << c.y+h << " " << c.z+h << "\n"; // v7

        int b = offset; // base index untuk voxel ini

        // 12 face (6 sisi × 2 segitiga)
        // Sisi bawah
        file << "f " << b+0 << " " << b+1 << " " << b+2 << "\n";
        file << "f " << b+0 << " " << b+2 << " " << b+3 << "\n";
        // Sisi atas
        file << "f " << b+4 << " " << b+6 << " " << b+5 << "\n";
        file << "f " << b+4 << " " << b+7 << " " << b+6 << "\n";
        // Sisi depan
        file << "f " << b+0 << " " << b+5 << " " << b+1 << "\n";
        file << "f " << b+0 << " " << b+4 << " " << b+5 << "\n";
        // Sisi belakang
        file << "f " << b+2 << " " << b+7 << " " << b+3 << "\n";
        file << "f " << b+2 << " " << b+6 << " " << b+7 << "\n";
        // Sisi kiri
        file << "f " << b+0 << " " << b+3 << " " << b+7 << "\n";
        file << "f " << b+0 << " " << b+7 << " " << b+4 << "\n";
        // Sisi kanan
        file << "f " << b+1 << " " << b+5 << " " << b+6 << "\n";
        file << "f " << b+1 << " " << b+6 << " " << b+2 << "\n";

        offset += 8; // geser offset untuk voxel berikutnya
        return;
    }

    // Bukan voxel → telusuri anak-anaknya
    for (int i = 0; i < 8; i++)
        collectVoxels(node->children[i], file, offset);
}
