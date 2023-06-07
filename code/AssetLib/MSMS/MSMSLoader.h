#pragma once

#include <assimp/BaseImporter.h>

namespace Assimp {
class MSMSImporter : public BaseImporter {
public:
    bool CanRead(const std::string &pFile, IOSystem *pIOHandler, bool checkSig) const override;

protected:
    const aiImporterDesc* GetInfo() const override;

    void InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) override;

private:
    /** Filename, for a verbose error message */
    std::string m_FileName;
};
} // namespace Assimp
