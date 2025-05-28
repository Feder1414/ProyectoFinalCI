
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "json.hpp"
#include <fstream>    




namespace Diligent
{
    using json = nlohmann::json;
class TiledMap
{
public:
    TiledMap() = default;
    enum class LayerType
    {
        FloorsWalls,
        Objects
    };

    struct TileInfo // info individual de un tile (desde el .tsx)
    {
        std::string Name;
        std::string Texture;
        int localId = 0; // id local dentro del tileset
        //int         localId = 0; // id dentro del tileset

        // --- NUEVO ---
        int tileWidth      = 16;
        int tileHeight     = 16;
        int imageWidth     = 256;
        int imageHeight    = 256;
        int tilesetColumns = 16;


    };

    struct ObjectInfo
    {
        uint32_t gid      = 0; // tile usado para identificar modelo
        float    x        = 0; // en pixels dentro del TMX
        float    y        = 0;
        float    rotY_deg = 0; // grados
        float    scale    = 1; // uniforme
        float    yOffset  = 0; // en unidades de mundo
        float	zOffset  = 0; // en unidades de mundo (no usado por ahora)
        float   xOffset  = 0; // en unidades de mundo (no usado por ahora)
    };


    const std::vector<ObjectInfo>& Objects() const noexcept { return m_Objects; }
    bool Load(const std::string& mapFile)
    {
        
               // std::filesystem::path mapPath{mapFile};
        // 1) lee el .json
        // Determinar carpeta del mapa
        auto          pos     = mapFile.find_last_of("/\\");
        std::string   baseDir = (pos == std::string::npos ? "" : mapFile.substr(0, pos + 1));

        std::ifstream ifs(mapFile);
        if (!ifs) return false;
        json j;
        ifs >> j;

        m_Width  = j["width"].get<int>();
        m_Height = j["height"].get<int>();

        // 2) reserva memoria
        m_FloorWall.resize(m_Width * m_Height, 0);
        m_Object.resize(m_Width * m_Height, 0);

        // 3) recorre capas
        for (auto& layer : j["layers"])
        {
            if (layer["type"] != "tilelayer" || !layer["visible"].get<bool>())
                continue;

            const std::string name   = layer["name"].get<std::string>();
            auto&             target = (name == "PisosParedes") ? m_FloorWall :
                            (name == "Objetos")                 ? m_Object :
                                                                  *(std::vector<uint32_t>*)nullptr;

            if (&target == nullptr) continue; // capa que no nos interesa

            const auto& data = layer["data"];
            for (size_t i = 0; i < data.size(); ++i)
                target[i] = data[i].get<uint32_t>();
        }

        for (auto& layer : j["layers"])
        {
            if (layer["type"] != "objectgroup" || !layer["visible"].get<bool>())
                continue; // no es la capa Objetos

            for (auto& obj : layer["objects"])
            {
                if (!obj.contains("gid")) continue; // solo nos interesan tile-objects

                ObjectInfo oi;
                oi.gid      = obj["gid"].get<uint32_t>();
                oi.x        = obj["x"].get<float>(); // px en Tiled (origen arriba-izq)
                oi.y        = obj["y"].get<float>();
                oi.rotY_deg = obj.value("rotation", 0.0f);

                // propiedades personalizadas -----------------------------------
                if (obj.contains("properties"))
                    for (auto& p : obj["properties"])
                    {
                        const std::string key = p["name"].get<std::string>();
                        if (key == "Scale") oi.scale = p["value"].get<float>();
                        if (key == "YOffset") oi.yOffset = p["value"].get<float>();
                        if (key == "RotY") oi.rotY_deg = p["value"].get<float>();
                        if (key == "ZOffset") oi.zOffset = p["value"].get<float>();
                        if (key == "XOffset") oi.xOffset = p["value"].get<float>();


                    }
                m_Objects.push_back(oi);
            }
        }

        OutputDebugStringA("Cargando tilesets \n");
        for (const auto& ts : j["tilesets"])
        {
            std::string src   = ts["source"].get<std::string>();
            uint32_t    first = ts["firstgid"].get<uint32_t>();
            //Outputdebug string que dice intentado cargar tileset
            OutputDebugStringA(("Cargando tileset: " + src + "\n").c_str());

           //auto tilesetPath = mapPath.parent_path() / src;

           std::string fullPath = baseDir + src;
  
           OutputDebugStringA((std::string{"Tileset fullPath: "} + fullPath + "\n").c_str());
            LoadTileset(fullPath, first);
        

        }

        return true;


    };                                               
    int Width() const noexcept { return m_Width; }   // n de celdas
    int Height() const noexcept { return m_Height; } // n de celdas

    /* Acceso rapido a ID crudo que devuelve Tiled (0 = vacío) */
    uint32_t GetTile(LayerType layer, int x, int y) const
    {
        const auto& v = (layer == LayerType::FloorsWalls) ? m_FloorWall : m_Object;
        return v[y * m_Width + x];
    }

    /* Acceso a las props del tile (si las había en el .tsx)   */
    const TileInfo* GetTileInfo(uint32_t gid) const
    {
        auto it = m_TileProps.find(gid);
        return (it == m_TileProps.end() ? nullptr : &it->second);
    }

private:
    void LoadTileset(const std::string& tsxFile, uint32_t firstGid)
    {
        OutputDebugStringA(("Cargando tileset: " + tsxFile + "\n").c_str());

        std::ifstream ifs(tsxFile);
        if (!ifs) { 
        OutputDebugStringA("Falla cargando el tileset\n");

            
        return; }

        json t;
        ifs >> t;
        if (!t.contains("tiles")) return;

        const int tw   = t["tilewidth"].get<int>();
        const int th   = t["tileheight"].get<int>();
        const int cols = t["columns"].get<int>();
        const int iw   = t["imagewidth"].get<int>();
        const int ih   = t["imageheight"].get<int>();



        for (auto& tile : t["tiles"])
        {
            uint32_t localId = tile["id"].get<uint32_t>();
            uint32_t gid     = firstGid + localId;

            TileInfo info;
            if (tile.contains("properties"))
            {
                for (auto& p : tile["properties"])
                {
                    const std::string key = p["name"].get<std::string>();
                    if (key == "Texture") info.Texture = p["value"].get<std::string>();
                    else if (key == "Name")
                        info.Name = p["value"].get<std::string>();
                }
            }
            m_TileProps[gid] = std::move(info);
            m_TileProps[gid].localId = localId; // guardar id local
            OutputDebugStringA(
				(std::to_string(gid) + " Name" + m_TileProps[gid].Name + "\n").c_str());

           
        }


    }

    int                                    m_Width  = 0;
    int                                    m_Height = 0;
    std::vector<uint32_t>                  m_FloorWall; // ids capa 1
    std::vector<uint32_t>                  m_Object;    // ids capa 2
    std::unordered_map<uint32_t, TileInfo> m_TileProps; // gid prop
    std::vector<ObjectInfo>                m_Objects;

};
} // namespace Diligent