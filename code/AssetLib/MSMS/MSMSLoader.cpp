#ifndef ASSIMP_BUILD_NO_MSMS_IMPORTER
#include "MSMSLoader.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <assimp/importerdesc.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/Exceptional.h>
#include <assimp/ParsingUtils.h>
#include <assimp/StringComparison.h>

static const aiImporterDesc desc = {
    "MSMS Molecule Mesh Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour,
    0,
    0,
    0,
    0,
    "msms"
};

void split(const std::string &s, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ' ')) {
        // Yes i hardcoded that space in there
        if(item != "")
            elems.push_back(item);
    }
}

std::vector<std::string> split(const std::string &s) {
    std::vector<std::string> elems;
    split(s, elems);
    return elems;
}

namespace Assimp {

/// Returns if @c s ends with @c suffix. If @c caseSensitive is false, both strings will be lower cased before matching.
static inline bool EndsWith(const std::string &s, const std::string &suffix, bool caseSensitive = true) {
    if (s.empty() || suffix.empty()) {
        return false;
    } else if (s.length() < suffix.length()) {
        return false;
    }

    if (!caseSensitive) {
        return EndsWith(ai_tolower(s), ai_tolower(suffix), true);
    }

    size_t len = suffix.length();
    std::string sSuffix = s.substr(s.length() - len, len);

    return (ASSIMP_stricmp(sSuffix, suffix) == 0);
}

const aiImporterDesc* MSMSImporter::GetInfo() const {
    return &desc;
}

bool MSMSImporter::CanRead(const std::string &pFile, [[maybe_unused]] Assimp::IOSystem *pIOHandler, [[maybe_unused]] bool checkSig) const {
    return EndsWith(pFile, ".msms", false);
}

void MSMSImporter::InternReadFile(const std::string &pFile, aiScene *pScene, [[maybe_unused]] Assimp::IOSystem *pIOHandler) {

    m_FileName = pFile;

    // Prepare scene
    pScene->mNumMeshes = 1;
    pScene->mMeshes = new aiMesh *[1];

    // Root node
    pScene->mRootNode = new aiNode();
    pScene->mRootNode->mName.Set(pFile);
    pScene->mRootNode->mNumMeshes = 1;
    pScene->mRootNode->mMeshes = new unsigned int[1];

    // false if face, true if vertex
    bool readingVertexSection = true;
    bool readingVertexHeader = false;
    bool readingFaceHeader = false;
    int headerCounter = 0;

    uint32_t vertexCount = 0;
    std::vector<aiVector3D> vertices;
    std::vector<aiVector3D> normals;
    std::vector<aiColor4D> colors;

    uint32_t faceCount = 0;
    std::vector<aiFace> faces;

    uint32_t sphereNum = 0;
    float density = 0.0f;
    float probeRadius = 0.0f;

    std::fstream newfile;
    newfile.open(pFile, std::ios::in);
    if (newfile.is_open())
    {
        std::string line;
        while(std::getline(newfile, line)) 
        {
            if(readingVertexHeader)
            {
                if(headerCounter == 1)
                {
                    // Last line of header contains information
                    std::vector<std::string> elems = split(line);
                    if(elems.size() != 4)
                    {
                        throw DeadlyImportError("Invalid vertex header in file ", pFile, ".");
                    }

                    vertexCount = std::stoi(elems[0]);
                    sphereNum = std::stoi(elems[1]);
                    density = std::stof(elems[2]);
                    probeRadius = std::stof(elems[3]);
                }

                headerCounter--;
                if(headerCounter == 0)
                {
                    readingVertexHeader = false;
                }
            }else if(readingFaceHeader)
            {
                if(headerCounter == 1)
                {
                    // Last line of header contains information
                    std::vector<std::string> elems = split(line);
                    if(elems.size() != 4)
                    {
                        throw DeadlyImportError("Invalid face header in file ", pFile, ".");
                    }

                    faceCount = std::stoi(elems[0]);
                }

                headerCounter--;
                if(headerCounter == 0)
                {
                    readingFaceHeader = false;
                }
            }else if(line == "=== Vertex Data ===")
            {
                readingVertexHeader = true;
                headerCounter = 3;
            }else if(line == "=== Face Data ===")
            {
                readingVertexSection = false;
                readingFaceHeader = true;
                headerCounter = 3;
            }else
            {
                if(readingVertexSection)
                {
                    std::vector<std::string> elems = split(line);
                    if(elems.size() != 9)
                    {
                        throw DeadlyImportError("Invalid vertex data in file ", pFile, ".");
                    }

                    aiVector3D vertex;
                    vertex.x = std::stof(elems[0]);
                    vertex.y = std::stof(elems[1]);
                    vertex.z = std::stof(elems[2]);
                    vertices.push_back(vertex);

                    aiVector3D normal;
                    normal.x = std::stof(elems[3]);
                    normal.y = std::stof(elems[4]);
                    normal.z = std::stof(elems[5]);
                    normals.push_back(normal);

                    aiColor4D color;
                    if(std::stoi(elems[8]) == 1)
                    {
                        color = {1.0f, 0.0f, 0.0f, 1.0f};
                    }else if(std::stoi(elems[8]) == 2)
                    {
                        color = {0.0f, 1.0f, 0.0f, 1.0f};
                    }else if(std::stoi(elems[8]) == 3)
                    {
                        color = {0.0f, 0.0f, 1.0f, 1.0f};
                    }
                    colors.push_back(color);

                }else
                {
                    std::vector<std::string> elems = split(line);
                    if(elems.size() != 5)
                    {
                        throw DeadlyImportError("Invalid vertex data in file ", pFile, ".");
                    }

                    aiFace face;
                    face.mNumIndices = 3;
                    face.mIndices = new unsigned int[3];
                    // MSMS starts indexing vertices at 1
                    face.mIndices[0] = std::stoi(elems[0]) - 1;
                    face.mIndices[1] = std::stoi(elems[1]) - 1;
                    face.mIndices[2] = std::stoi(elems[2]) - 1;
                    faces.push_back(face);
                }
            }
        }

        newfile.close();
    }else
    {
        throw DeadlyImportError("Failed to open file ", pFile, ".");
    }

    // Set mesh data
    aiMesh* mesh = new aiMesh();
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;    

    // Vertices
    mesh->mNumVertices = vertexCount;
    mesh->mVertices = new aiVector3D[mesh->mNumVertices];
    mesh->mNormals = new aiVector3D[mesh->mNumVertices];
    mesh->mColors[0] = new aiColor4D[mesh->mNumVertices];

    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        mesh->mVertices[i] = vertices[i];
        mesh->mNormals[i] = normals[i]; 
        mesh->mColors[0][i] = colors[i];
    }

    // Faces
    mesh->mNumFaces = faceCount;
    mesh->mFaces = new aiFace[mesh->mNumFaces];

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        mesh->mFaces[i] = faces[i];
    }

    // Add mesh to scene
    pScene->mMeshes[0] = mesh;
    pScene->mRootNode->mMeshes[0] = static_cast<unsigned int>(0);
}

} // namespace Assimp

#endif
