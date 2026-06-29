#include <cstddef>
#include <iostream>

#include <memlib/memlib.hpp>

struct vec3{
    float x, y, z;
};

struct Player{
    vec3 pos;
    int health;
};

int main(){
    Player p{
        .pos = vec3{
            .x = 3,
            .y = 4,
            .z = 5
        },
        .health = 100
    };

    if(auto res = memlib::Context::internal()){
        auto ctx = std::move(res).value();

        auto addr = (memlib::address_t)&p;

        auto mem = ctx.at(addr);

        if(auto _res = mem.read<vec3>()){
            auto vec = std::move(_res).value();
            std::cout << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << std::endl;
        }

        std::cout << "Address: " << (void*)mem.address() << std::endl;

        mem.add_inplace(offsetof(Player, health));

        if(auto _res = mem.read<int>()){
            auto health = std::move(_res).value();
            std::cout << "health: " << health << std::endl;
        }
        if(mem.write<int>(67)){
            std::cout << "health: " << p.health << std::endl;
        }
        std::cout << "Address: " << (void*)mem.address() << std::endl;
    }
}