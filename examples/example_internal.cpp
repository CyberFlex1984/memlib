#include <cstddef>
#include <print>

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
            std::println("x: {}, y: {}, z: {}", vec.x, vec.y, vec.z);
        }

        std::println("Address: 0x{:x}", mem.address());

        mem.add_inplace(offsetof(Player, health));

        if(auto _res = mem.read<int>()){
            auto health = std::move(_res).value();
            std::println("health: {}", health);
        }
        if(mem.write<int>(67)){
            std::println("health: {}", p.health);
        }
        std::println("Address: 0x{:x}", mem.address());
    }
}