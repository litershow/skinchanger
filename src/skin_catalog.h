#pragma once

#include <cstdint>
#include <vector>

struct CatalogSkin
{
    const char* name;
    int paintKit;
    int seed;
    float wear;
    int statTrak;
};

struct CatalogWeapon
{
    const char* name;
    std::uint16_t itemDefinition;
    std::vector<CatalogSkin> skins;
};

struct CatalogGroup
{
    const char* name;
    std::vector<CatalogWeapon> weapons;
};

const std::vector<CatalogGroup>& GetSkinCatalog();
