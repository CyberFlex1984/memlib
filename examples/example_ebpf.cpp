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

    if(auto internal_ctx_res = memlib::Context::internal()){
        auto internal_ctx = std::move(internal_ctx_res).value();
        auto crnt_pid = internal_ctx.get_pid().value(); 
        // INTERNAL CONTEXT ONLY FOR GETTING PID OF THIS PROCESS
        auto external_ctx_res = memlib::Context::ebpf(crnt_pid);
        if(external_ctx_res){
            // sample code as in internal
            auto ctx = std::move(external_ctx_res).value();

            std::cout << std::endl;

            if(auto modules_res = ctx.get_modules()){
                auto modules = std::move(modules_res).value();
                std::cout << modules.size() << " modules!" << std::endl;
                for(const auto& module : modules) {
                    std::cout << module.name << ": " << (void*)module.base << " size: " << module.size << std::endl;
                }
            }

            std::cout << std::endl;

            auto addr = (memlib::address_t)&p; // It's a sample addr

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

            // if u'll get modules here it can crash, I don't know why
        }
        else{
            std::cout << memlib::to_string(external_ctx_res.error()) << std::endl;
        }
    }
}