#include <memlib/memlib.hpp>

struct vec3{
    float x, y, z;
};

struct Player{
    vec3 pos;
    int health;
};

int main(){
    if(auto res = memlib::Context::internal()){
        auto ctx = std::move(res).value();

        
    }
}