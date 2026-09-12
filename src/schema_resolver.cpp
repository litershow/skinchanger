#include "schema_resolver.h"

#include <cstring>

namespace
{
constexpr std::ptrdiff_t kInvalidOffset = -1;
}

void SchemaResolver::Initialize(ISchemaSystem* schemaSystem)
{
    m_schemaSystem = schemaSystem;
    m_serverScope = nullptr;
    m_cache.clear();

    if (!m_schemaSystem)
        return;

#ifdef _WIN32
    const char* candidates[] = { "server.dll", "server" };
#else
    const char* candidates[] = { "libserver.so", "server" };
#endif

    for (const char* module : candidates)
    {
        m_serverScope = m_schemaSystem->FindTypeScopeForModule(module);
        if (m_serverScope)
            break;
    }
}

void SchemaResolver::ResetCache()
{
    m_cache.clear();
}

std::ptrdiff_t SchemaResolver::FindOffset(const char* className, const char* fieldName)
{
    if (!m_serverScope || !className || !fieldName)
        return kInvalidOffset;

    const std::string key = std::string(className) + "::" + fieldName;
    auto it = m_cache.find(key);
    if (it != m_cache.end())
        return it->second;

    CSchemaClassInfo* classInfo = m_serverScope->FindRawClassBinding(className);
    const std::ptrdiff_t offset = FindOffset(classInfo, fieldName);
    m_cache.emplace(key, offset);
    return offset;
}

std::ptrdiff_t SchemaResolver::FindOffset(CSchemaClassInfo* classInfo, const char* fieldName)
{
    if (!classInfo || !fieldName)
        return kInvalidOffset;
    return FindOffsetRecursive(classInfo, fieldName, 0);
}

std::ptrdiff_t SchemaResolver::FindOffsetRecursive(CSchemaClassInfo* classInfo, const char* fieldName, int depth)
{
    if (!classInfo || !fieldName || depth > 32)
        return kInvalidOffset;

    for (uint16 i = 0; i < classInfo->m_nFieldCount; ++i)
    {
        const SchemaClassFieldData_t& field = classInfo->m_pFields[i];
        if (field.m_pszName && std::strcmp(field.m_pszName, fieldName) == 0)
            return field.m_nSingleInheritanceOffset;
    }

    for (uint8 i = 0; i < classInfo->m_nBaseClassCount; ++i)
    {
        const SchemaBaseClassInfoData_t& base = classInfo->m_pBaseClasses[i];
        const std::ptrdiff_t nested = FindOffsetRecursive(base.m_pClass, fieldName, depth + 1);
        if (nested >= 0)
            return static_cast<std::ptrdiff_t>(base.m_nOffset) + nested;
    }

    return kInvalidOffset;
}
