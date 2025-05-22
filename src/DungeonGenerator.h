#pragma once
#include <vector>
#include <random>
#include <cstdint>
#include <algorithm>
#include <optional>
#include <sstream>     
#include <iostream>    
#include <fstream> 



class DungeonGenerator
{
public:
    DungeonGenerator() = default;

    enum class Tile : uint8_t
    {
        Empty = 0, // vacio
        Floor = 1, // piso
        Wall  = 2  // pared
    };
    struct Room
    {
        int x, y, w, h;
    };

    struct Rect
    {
        int x, y, w, h;

        // Leaf = todavia no dividido
        bool IsLeaf() const noexcept { return left == nullptr && right == nullptr; }

        // Hijos creados luego de un corte
        std::unique_ptr<Rect> left;
        std::unique_ptr<Rect> right;

        
        // Sala final tallada dentro de esta hoja
        bool                  hasRoom = false;
        std::unique_ptr<Rect> room;
    };

    

    
    void Generate(int w, int h, int minLeaf = 8, int maxLeaf = 20, uint32_t seed = std::random_device{}())
    {
        Width  = w;
        Height = h;
        _grid.assign(Height, std::vector<Tile>(Width, Tile::Wall)); // start full of walls

        rng.seed(seed);
        _root = std::make_unique<Rect>(Rect{0, 0, Width, Height});

        SplitLeaf(*_root, minLeaf, maxLeaf);
        CreateRooms(*_root);
        DigCorridors(*_root);
    }

    Tile GetTile(int x, int y) const noexcept { return _grid[y][x]; }
    int  GetWidth() const noexcept { return Width; }
    int  GetHeight() const noexcept { return Height; }

     std::string ToString() const
    {
        std::ostringstream oss;
        for (int y = 0; y < Height; ++y)
        {
            for (int x = 0; x < Width; ++x)
                oss << TileToChar(GetTile(x, y));
            oss << '\n';
        }
        return oss.str();
    }

    //
    void DumpToConsole() const
    {
        std::cout << ToString();
    }

    //Guarda una imagen PPM (P6) de 1 pixel por celda.
    bool SavePPM(const std::string& filename) const
    {
        std::ofstream ofs(filename, std::ios::binary);
        if (!ofs) return false;

        ofs << "P6\n"
            << Width << ' ' << Height << "\n255\n";
        for (int y = 0; y < Height; ++y)
        {
            for (int x = 0; x < Width; ++x)
            {
                unsigned char rgb[3];
                switch (GetTile(x, y))
                {
                    case Tile::Floor: rgb[0] = rgb[1] = rgb[2] = 200; break; // gris claro
                    case Tile::Wall: rgb[0] = rgb[1] = rgb[2] = 80; break;   // gris oscuro
                    default: rgb[0] = rgb[1] = rgb[2] = 0; break;            // negro
                }
                ofs.write(reinterpret_cast<char*>(rgb), 3);
            }
        }
        OutputDebugStringA("Generación completada :D");
        return true;
    }

     


private:

    static char TileToChar(Tile t) noexcept
    {
        switch (t)
        {
            case Tile::Floor: return '.';
            case Tile::Wall: return '#';
            default: return ' ';
        }
    }

    int                            Width  = 0;
    int                            Height = 0;
    std::vector<std::vector<Tile>> _grid;

    std::mt19937          rng;
    std::unique_ptr<Rect> _root;

    std::vector<std::vector<Tile>> getGrid() const { return _grid; } 


    // Corte recursido de BPS
    bool SplitLeaf(Rect& leaf, int minLeaf, int maxLeaf)
    {
        if (!leaf.IsLeaf())
            return false; // ya cortado

        bool splitH = RandomBool();
        if (leaf.w > leaf.h && leaf.w / leaf.h >= 1.25f)
            splitH = false;
        else if (leaf.h > leaf.w && leaf.h / leaf.w >= 1.25f)
            splitH = true;

        int max = (splitH ? leaf.h : leaf.w) - minLeaf;
        if (max <= minLeaf)
            return false; // muy pequeño para cortar

        std::uniform_int_distribution<int> dist(minLeaf, std::min(maxLeaf, max));
        int                                split = dist(rng);

        if (splitH)
        {
            leaf.left  = std::make_unique<Rect>(Rect{leaf.x, leaf.y, leaf.w, split});
            leaf.right = std::make_unique<Rect>(Rect{leaf.x, leaf.y + split, leaf.w, leaf.h - split});
        }
        else
        {
            leaf.left  = std::make_unique<Rect>(Rect{leaf.x, leaf.y, split, leaf.h});
            leaf.right = std::make_unique<Rect>(Rect{leaf.x + split, leaf.y, leaf.w - split, leaf.h});
        }

        SplitLeaf(*leaf.left, minLeaf, maxLeaf);
        SplitLeaf(*leaf.right, minLeaf, maxLeaf);
        return true;
    }

 
    // Tallar una sala en cada hoja
    void CreateRooms(Rect& leaf)
    {
        if (!leaf.IsLeaf())
        {
            CreateRooms(*leaf.left);
            CreateRooms(*leaf.right);
            return;
        }

        // Room size at least 3x3
        std::uniform_int_distribution<int> rw(3, leaf.w - 2);
        std::uniform_int_distribution<int> rh(3, leaf.h - 2);
        int                                roomW = rw(rng);
        int                                roomH = rh(rng);

        std::uniform_int_distribution<int> rx(1, leaf.w - roomW - 1);
        std::uniform_int_distribution<int> ry(1, leaf.h - roomH - 1);
        int                                roomX = leaf.x + rx(rng);
        int                                roomY = leaf.y + ry(rng);

        leaf.room = std::make_unique<Rect>(Rect{roomX, roomY, roomW, roomH});
        leaf.hasRoom = true; 

        for (int y = roomY; y < roomY + roomH; ++y)
            for (int x = roomX; x < roomX + roomW; ++x)
                _grid[y][x] = Tile::Floor;
    }

  
    // Conectar hojas hermanas con un pasillo en forma de L
    void DigCorridors(Rect& leaf)
    {
        if (!leaf.left || !leaf.right)
            return;

        DigCorridors(*leaf.left);
        DigCorridors(*leaf.right);

        Point p1 = ChoosePointInRoom(*leaf.left);
        Point p2 = ChoosePointInRoom(*leaf.right);

        if (RandomBool())
        {
            CarveHorizontal(p1.x, p2.x, p1.y);
            CarveVertical(p1.y, p2.y, p2.x);
        }
        else
        {
            CarveVertical(p1.y, p2.y, p1.x);
            CarveHorizontal(p1.x, p2.x, p2.y);
        }
    }

    struct Point
    {
        int x, y;
    };

    //Point ChoosePointInRoom(const Rect& leaf)
    //{
    //    const Rect&                        r = leaf.room; // ¡leaf.hasRoom debe ser true!
    //    std::uniform_int_distribution<int> dx(r.x, r.x + r.w - 1);
    //    std::uniform_int_distribution<int> dy(r.y, r.y + r.h - 1);
    //    return {dx(rng), dy(rng)};
    //}

    Point ChoosePointInRoom(const Rect& node)
    {
        // Desciende hasta un hijo con sala
        const Rect* cur = &node;
        while (!cur->hasRoom)
        {
            // ambos hijos existen
            cur = RandomBool() ? cur->left.get() : cur->right.get();
        }

        const Rect&                        r = *cur->room; // aquí sí hay sala
        std::uniform_int_distribution<int> dx(r.x, r.x + r.w - 1);
        std::uniform_int_distribution<int> dy(r.y, r.y + r.h - 1);
        return {dx(rng), dy(rng)};
    }


    void CarveHorizontal(int x1, int x2, int y)
    {
        if (x2 < x1) std::swap(x1, x2);
        for (int x = x1; x <= x2; ++x)
            _grid[y][x] = Tile::Floor;
    }

    void CarveVertical(int y1, int y2, int x)
    {
        if (y2 < y1) std::swap(y1, y2);
        for (int y = y1; y <= y2; ++y)
            _grid[y][x] = Tile::Floor;
    }

    bool RandomBool() { return std::uniform_int_distribution<int>(0, 1)(rng) == 1; }


};
