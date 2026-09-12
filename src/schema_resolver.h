#pragma once

#include <schemasystem/schemasystem.h>
#include <schemasystem/schematypes.h>

#include <cstddef>
#include <string>
#include <unordered_map>

class SchemaResolver
{
public:
    void Initialize(ISchemaSystem* schemaSystem);
    void ResetCache();

    // Finds an offset from a named class in server.dll/libserver.so.
    std::ptrdiff_t FindOffset(const char* className, const char* fieldName);

    // Finds an offset using an entity's runtime dynamic binding, including base classes.
    std::ptrdiff_t FindOffset(CSchemaClassInfo* classInfo, const char* fieldName);

    bool IsReady() const { return m_serverScope != nullptr; }

private:
    std::ptrdiff_t FindOffsetRecursive(CSchemaClassInfo* classInfo, const char* fieldName, int depth);

    ISchemaSystem* m_schemaSystem = nullptr;
    CSchemaSystemTypeScope* m_serverScope = nullptr;
    std::unordered_map<std::string, std::ptrdiff_t> m_cache;
};
