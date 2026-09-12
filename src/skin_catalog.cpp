#include "skin_catalog.h"

const std::vector<CatalogGroup>& GetSkinCatalog()
{
    static const std::vector<CatalogGroup> catalog = {
        {"Pistols", {
            {"Desert Eagle", 1, {
                {"Default", 0, 0, 0.0001f, -1}, {"Blaze", 37, 0, 0.0001f, -1},
                {"Kumicho Dragon", 527, 0, 0.0001f, -1}, {"Code Red", 711, 0, 0.0001f, -1},
                {"Printstream", 962, 0, 0.0001f, -1}, {"Ocean Drive", 1021, 0, 0.0001f, -1}
            }},
            {"Glock-18", 4, {
                {"Default", 0, 0, 0.0001f, -1}, {"Fade", 38, 0, 0.0001f, -1},
                {"Water Elemental", 353, 0, 0.0001f, -1}, {"Bullet Queen", 957, 0, 0.0001f, -1},
                {"Vogue", 963, 0, 0.0001f, -1}
            }},
            {"USP-S", 61, {
                {"Default", 0, 0, 0.0001f, -1}, {"Kill Confirmed", 504, 0, 0.0001f, -1},
                {"Cortex", 705, 0, 0.0001f, -1}, {"The Traitor", 1040, 0, 0.0001f, -1},
                {"Printstream", 1142, 0, 0.0001f, -1}
            }},
            {"P250", 36, {
                {"Default", 0, 0, 0.0001f, -1}, {"Muertos", 404, 0, 0.0001f, -1},
                {"Asiimov", 551, 0, 0.0001f, -1}, {"See Ya Later", 678, 0, 0.0001f, -1}
            }},
            {"Five-SeveN", 3, {
                {"Default", 0, 0, 0.0001f, -1}, {"Monkey Business", 427, 0, 0.0001f, -1},
                {"Hyper Beast", 660, 0, 0.0001f, -1}, {"Angry Mob", 837, 0, 0.0001f, -1}
            }},
            {"Tec-9", 30, {
                {"Default", 0, 0, 0.0001f, -1}, {"Fuel Injector", 614, 0, 0.0001f, -1},
                {"Decimator", 889, 0, 0.0001f, -1}
            }},
            {"CZ75-Auto", 63, {
                {"Default", 0, 0, 0.0001f, -1}, {"Victoria", 270, 0, 0.0001f, -1},
                {"Xiangliu", 643, 0, 0.0001f, -1}
            }},
            {"R8 Revolver", 64, {
                {"Default", 0, 0, 0.0001f, -1}, {"Fade", 522, 0, 0.0001f, -1},
                {"Amber Fade", 523, 0, 0.0001f, -1}
            }}
        }},
        {"Rifles", {
            {"AK-47", 7, {
                {"Default", 0, 0, 0.0001f, -1}, {"Redline", 282, 0, 0.0001f, -1},
                {"Vulcan", 302, 0, 0.0001f, -1}, {"Fuel Injector", 524, 0, 0.0001f, -1},
                {"Bloodsport", 639, 0, 0.0001f, -1}, {"Neon Rider", 707, 0, 0.0001f, -1},
                {"Asiimov", 801, 0, 0.0001f, -1}, {"Slate", 1035, 0, 0.0001f, -1}
            }},
            {"M4A4", 16, {
                {"Default", 0, 0, 0.0001f, -1}, {"Asiimov", 255, 0, 0.0001f, -1},
                {"Howl", 309, 0, 0.0001f, -1}, {"Desolate Space", 588, 0, 0.0001f, -1},
                {"Neo-Noir", 695, 0, 0.0001f, -1}, {"Temukau", 1089, 0, 0.0001f, -1}
            }},
            {"M4A1-S", 60, {
                {"Default", 0, 0, 0.0001f, -1}, {"Hyper Beast", 430, 0, 0.0001f, -1},
                {"Golden Coil", 497, 0, 0.0001f, -1}, {"Decimator", 644, 0, 0.0001f, -1},
                {"Player Two", 946, 0, 0.0001f, -1}, {"Printstream", 984, 0, 0.0001f, -1}
            }},
            {"Galil AR", 13, {
                {"Default", 0, 0, 0.0001f, -1}, {"Chatterbox", 398, 0, 0.0001f, -1},
                {"Eco", 428, 0, 0.0001f, -1}, {"Sugar Rush", 661, 0, 0.0001f, -1}
            }},
            {"FAMAS", 10, {
                {"Default", 0, 0, 0.0001f, -1}, {"Roll Cage", 604, 0, 0.0001f, -1},
                {"Mecha Industries", 626, 0, 0.0001f, -1}, {"Commemoration", 919, 0, 0.0001f, -1}
            }},
            {"AUG", 8, {
                {"Default", 0, 0, 0.0001f, -1}, {"Akihabara Accept", 455, 0, 0.0001f, -1},
                {"Syd Mead", 601, 0, 0.0001f, -1}, {"Momentum", 845, 0, 0.0001f, -1}
            }},
            {"SG 553", 39, {
                {"Default", 0, 0, 0.0001f, -1}, {"Cyrex", 487, 0, 0.0001f, -1},
                {"Integrale", 750, 0, 0.0001f, -1}, {"Colony IV", 897, 0, 0.0001f, -1}
            }}
        }},
        {"Sniper Rifles", {
            {"AWP", 9, {
                {"Default", 0, 0, 0.0001f, -1}, {"Asiimov", 279, 0, 0.0001f, -1},
                {"Dragon Lore", 344, 0, 0.0001f, -1}, {"Hyper Beast", 475, 0, 0.0001f, -1},
                {"Gungnir", 756, 0, 0.0001f, -1}, {"Neo-Noir", 803, 0, 0.0001f, -1},
                {"Containment Breach", 887, 0, 0.0001f, -1}
            }},
            {"SSG 08", 40, {
                {"Default", 0, 0, 0.0001f, -1}, {"Blood in the Water", 222, 0, 0.0001f, -1},
                {"Dragonfire", 624, 0, 0.0001f, -1}, {"Turbo Peek", 1101, 0, 0.0001f, -1}
            }},
            {"SCAR-20", 38, {
                {"Default", 0, 0, 0.0001f, -1}, {"Cyrex", 312, 0, 0.0001f, -1},
                {"Bloodsport", 597, 0, 0.0001f, -1}
            }},
            {"G3SG1", 11, {
                {"Default", 0, 0, 0.0001f, -1}, {"The Executioner", 511, 0, 0.0001f, -1},
                {"Flux", 493, 0, 0.0001f, -1}
            }}
        }},
        {"SMGs", {
            {"MAC-10", 17, {
                {"Default", 0, 0, 0.0001f, -1}, {"Neon Rider", 433, 0, 0.0001f, -1},
                {"Stalker", 898, 0, 0.0001f, -1}, {"Disco Tech", 947, 0, 0.0001f, -1}
            }},
            {"MP9", 34, {
                {"Default", 0, 0, 0.0001f, -1}, {"Food Chain", 1037, 0, 0.0001f, -1},
                {"Mount Fuji", 1095, 0, 0.0001f, -1}, {"Starlight Protector", 1022, 0, 0.0001f, -1}
            }},
            {"P90", 19, {
                {"Default", 0, 0, 0.0001f, -1}, {"Death by Kitty", 156, 0, 0.0001f, -1},
                {"Emerald Dragon", 182, 0, 0.0001f, -1}, {"Asiimov", 359, 0, 0.0001f, -1}
            }},
            {"UMP-45", 24, {
                {"Default", 0, 0, 0.0001f, -1}, {"Primal Saber", 556, 0, 0.0001f, -1},
                {"Momentum", 802, 0, 0.0001f, -1}
            }},
            {"MP7", 33, {
                {"Default", 0, 0, 0.0001f, -1}, {"Bloodsport", 696, 0, 0.0001f, -1},
                {"Neon Ply", 893, 0, 0.0001f, -1}
            }},
            {"MP5-SD", 23, {
                {"Default", 0, 0, 0.0001f, -1}, {"Phosphor", 810, 0, 0.0001f, -1},
                {"Kitbash", 974, 0, 0.0001f, -1}
            }},
            {"PP-Bizon", 26, {
                {"Default", 0, 0, 0.0001f, -1}, {"Judgement of Anubis", 542, 0, 0.0001f, -1},
                {"High Roller", 676, 0, 0.0001f, -1}
            }}
        }},
        {"Heavy", {
            {"XM1014", 25, {
                {"Default", 0, 0, 0.0001f, -1}, {"Tranquility", 393, 0, 0.0001f, -1},
                {"Seasons", 654, 0, 0.0001f, -1}, {"Incinegator", 850, 0, 0.0001f, -1}
            }},
            {"MAG-7", 27, {
                {"Default", 0, 0, 0.0001f, -1}, {"Bulldozer", 39, 0, 0.0001f, -1},
                {"Justice", 948, 0, 0.0001f, -1}
            }},
            {"Nova", 35, {
                {"Default", 0, 0, 0.0001f, -1}, {"Hyper Beast", 537, 0, 0.0001f, -1},
                {"Wild Six", 699, 0, 0.0001f, -1}
            }},
            {"Negev", 28, {
                {"Default", 0, 0, 0.0001f, -1}, {"Power Loader", 514, 0, 0.0001f, -1},
                {"Mjolnir", 763, 0, 0.0001f, -1}
            }},
            {"M249", 14, {
                {"Default", 0, 0, 0.0001f, -1}, {"Nebula Crusader", 496, 0, 0.0001f, -1},
                {"Emerald Poison Dart", 648, 0, 0.0001f, -1}
            }}
        }}
    };
    return catalog;
}
