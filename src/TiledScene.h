#pragma once
#include "TiledMap.h"
#include "Cubo.h"         // cubo Diligent que ya tienes
#include "GLTFLoader.hpp" // para modelos
#include <unordered_map>


namespace Diligent
{

/*  tipos auxiliares  -*/
struct TileDraw
{
    float4x4 World;
    uint32_t MaterialId; // 0=suelo  1=muro  
};

struct ObjectDraw
{
    float4x4     World;
    GLTF::Model* pModel; // modelo que debes renderizar
};

struct TileVertex
{
    float3 pos;
    float3 normal;
    float2 uv;
    float4 tangent; // .xyz = tangent vector, .w = signo para bitangent

};


/*
  TileScene:
    Recorre un TiledMap y crea dos listas:
        - Instancias de cubo/plano   (pisos muros)
        - Instancias de modelos GLTF (objetos)
    Decide qué es cada cosa leyendo las propiedades
      Name o Texture del tileset.
 -*/
class TileScene
{
public:
    /// @param tileSize     ancho/largo de cada celda en mundo
    /// @param wallHeight   altura del muro
    TileScene(float tileSize   = 2.f,
              float wallHeight = 2.f,
              float floorThick = 0.1f) :
        m_TileSize{tileSize},
        m_WallHeight{wallHeight},
        m_FloorThickness{floorThick}
    {}

    /* Construye la escena.
       - modelLookup    relaciona Name-of-tile    GLTF::Model*
       - floorMatId / wallMatId  coinciden con tus materiales (0/1)       */
    void Build(const TiledMap&                         map,
                          const std::unordered_map<std::string,
                                                   GLTF::Model*>& modelLookup,
                          uint32_t                                floorMatId,
                          uint32_t                                wallMatId)
    {
        m_Tiles.clear();
        m_Objects.clear();

        const int   W    = map.Width();
        const int   H    = map.Height();
        const float TS   = m_TileSize;
        const float xOff = -W * TS * 0.5f + TS * 0.5f;
        const float zOff = -H * TS * 0.5f + TS * 0.5f;

        //------------------  capa pisos / paredes  -------------------------
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
            {
                uint32_t gid = map.GetTile(TiledMap::LayerType::FloorsWalls, x, y);
                if (gid == 0) continue;

                const auto* info    = map.GetTileInfo(gid);


                bool        isFloor = info && info->Name == "Floor";
                bool        isWall  = info && info->Name == "Wall";
                if (!isFloor && !isWall) isWall = true; // por defecto muro

                float    wx = x * TS + xOff;
                float    wz = y * TS + zOff;
                float4x4 S, T;

                if (isFloor)
                {
                    S = float4x4::Scale(
                        float3{TS * 0.5f, m_FloorThickness * 0.5f, TS * 0.5f});
                    T = float4x4::Translation(
                        float3{wx, -m_WallHeight * 0.5f + m_FloorThickness * 0.5f, wz});
                    m_Tiles.push_back({S * T, floorMatId});
                }
                else // wall
                {
                    S = float4x4::Scale(
                        float3{TS * 0.5f, m_WallHeight * 0.5f, TS * 0.5f});
                    T = float4x4::Translation(float3{wx, 0.f, wz});
                    m_Tiles.push_back({S * T, wallMatId});
                }
            }

        for (auto& parNombreObjeto: modelLookup){
            OutputDebugStringA(("Modelo: " + parNombreObjeto.first + "\n").c_str());
        
        }

     //   //------------------  capa objetos  ---------------------------------
     //   for (int y = 0; y < H; ++y)
     //       for (int x = 0; x < W; ++x)
     //       {
     //           uint32_t gid = map.GetTile(TiledMap::LayerType::Objects, x, y);
     //           if (gid == 0) continue;

     //           const auto* info = map.GetTileInfo(gid);
     //           if (!info) continue;
     //           OutputDebugStringA(
					//("Objeto: " + info->Name + " en " + std::to_string(x) + "," +
					// std::to_string(y) + "\n")
					//	.c_str());
     //           auto it = modelLookup.find(info->Name);
     //           if (it == modelLookup.end()) continue; // no hay modelo

     //           float    wx = x * TS + xOff;
     //           float    wz = y * TS + zOff;
     //           float4x4 Wm = float4x4::Translation(float3{wx, 0.f, wz});
     //           m_Objects.push_back({Wm, it->second});
     //           OutputDebugStringA("A");
     //       }


        for (const auto& oi : map.Objects())
        {
            const auto* info = map.GetTileInfo(oi.gid);
            int         tw   = info->tileWidth;
            int         th   = info->tileHeight;

            if (!info) continue; // por si hay un gid huérfano

            auto it = modelLookup.find(info->Name);
            if (it == modelLookup.end())
                continue; // no tenemos ese modelo

            //// ----- convertir coordenadas de Tiled (px) a mundo -----------
            //const float TS = m_TileSize;
            //float       wx = -(oi.x / tw - 0.5f) * TS;  // tw = ancho tile en px
            //float       wz = -(oi.y / th - 0.5f) * TS; // inversión eje Y
            //float       wy = oi.yOffset;               // elevación opcional

            // ---- datos base ---------------------------------------------------
            const float TS   = m_TileSize; // tamaño de un tile en mundo
            const float xOff = -W * TS * 0.5f + TS * 0.5f;
            const float zOff = -H * TS * 0.5f + TS * 0.5f;


           float col = oi.x / float(tw);        // columna
            float row = (oi.y - th) / float(th); // F I J A T E  aquí restamos th

            float wx = (col + 0.5f) * TS + xOff + oi.xOffset; // centro de la celda
            float wz = (row + 0.5f) * TS + zOff + oi.zOffset;
            float wy = oi.yOffset; // elevación opcional


            float4x4 S   = float4x4::Scale(float3{oi.scale});
            float    rad = oi.rotY_deg * PI_F / 180.f;
            float4x4 R   = float4x4::RotationY(rad);
            float4x4 T   = float4x4::Translation(float3{wx, wy, wz});

            m_Objects.push_back({S * R * T, it->second});
        }
    }

    enum class Want
    {
        Floor,
        Wall
    };


    void BuildCombinedMesh(const TiledMap&          map,
                           TiledMap::LayerType      layer,
                           Want what,
              
                           std::vector<TileVertex>& outVerts,
                           std::vector<uint32_t>&   outIdx)
    {
        const int   W    = map.Width();
        const int   H    = map.Height();
        const float TS   = m_TileSize;
        const float xOff = -W * TS * 0.5f + TS * 0.5f;
        const float zOff = -H * TS * 0.5f + TS * 0.5f;

        // normales y tangentes constantes para suelo horizontal
        const float3 floorNormal  = {0, 1, 0};
        const float4 floorTangent = {1, 0, 0, 1}; // bitangent = normal × tangent * w

        for (int y = 0; y < H; ++y)
        {
            for (int x = 0; x < W; ++x)
            {
                uint32_t gid = map.GetTile(layer, x, y);
                if (gid == 0) continue;
                auto info = map.GetTileInfo(gid);
                if (!info) continue;

                
            bool isFloor = info->Name == "Floor";
                bool isWall  = info->Name == "Wall" || !isFloor;

                if ((what == Want::Floor && !isFloor) ||
                    (what == Want::Wall && !isWall))
                    continue;    


                // calcula UVs (igual que antes)…
                int   cols = info->tilesetColumns;
                float tw = info->tileWidth, th = info->tileHeight;
                float iw = info->imageWidth, ih = info->imageHeight;
                int   li = info->localId;
                int   cx = li % cols, cy = li / cols;
                /*float u0 = cx * tw / iw, u1 = (cx + 1) * tw / iw;
                float v0 = cy * th / ih, v1 = (cy + 1) * th / ih;*/

                float wx = x * TS + xOff;
                float wz = y * TS + zOff;
                float hx = TS * 0.5f;

                // cuatro esquinas
                float3 p0{wx - hx, 0, wz - hx};
                float3 p1{wx + hx, 0, wz - hx};
                float3 p2{wx + hx, 0, wz + hx};
                float3 p3{wx - hx, 0, wz + hx};

                uint32_t base = (uint32_t)outVerts.size();
                //// P0
                //outVerts.push_back({p0, floorNormal, floorTangent, {u0, v0}});
                //outVerts.push_back({p1, floorNormal, floorTangent, {u1, v0}});
                //outVerts.push_back({p2, floorNormal, floorTangent, {u1, v1}});
                //outVerts.push_back({p3, floorNormal, floorTangent, {u0, v1}});
                float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;

                // en lugar de u0=0…1 cada tile, calcula UV desde la posición en mundo:
                float2 uv0 = {(wx - hx) / TS, (wz - hx) / TS};
                float2 uv1 = {(wx + hx) / TS, (wz - hx) / TS};
                float2 uv2 = {(wx + hx) / TS, (wz + hx) / TS};
                float2 uv3 = {(wx - hx) / TS, (wz + hx) / TS};



                // P0
                outVerts.push_back({p0, floorNormal, {u0, v0}, floorTangent});
                // P1
                outVerts.push_back({p1, floorNormal, {u1, v0}, floorTangent});
                // P2
                outVerts.push_back({p2, floorNormal, {u1, v1}, floorTangent});
                // P3
                outVerts.push_back({p3, floorNormal, {u0, v1}, floorTangent});

               /* outVerts.push_back({p0, floorNormal, uv0, floorTangent});
                outVerts.push_back({p1, floorNormal, uv1, floorTangent});
                outVerts.push_back({p2, floorNormal, uv2, floorTangent});
                outVerts.push_back({p3, floorNormal, uv3, floorTangent});*/


                // dos triángulos
                outIdx.push_back(base + 0);
                outIdx.push_back(base + 1);
                outIdx.push_back(base + 2);
                outIdx.push_back(base + 0);
                outIdx.push_back(base + 2);
                outIdx.push_back(base + 3);
            }
        }
    }


    /* acceso a los datos ya listos para tu render loop */
    const std::vector<TileDraw>&   Tiles() const noexcept { return m_Tiles; }
    const std::vector<ObjectDraw>& Objects() const noexcept { return m_Objects; }

private:
    float m_TileSize;
    float m_WallHeight;
    float m_FloorThickness;

    std::vector<TileDraw>   m_Tiles;
    std::vector<ObjectDraw> m_Objects;
};

} // namespace Diligent
