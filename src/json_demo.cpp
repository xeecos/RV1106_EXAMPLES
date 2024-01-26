#include "json.hpp"
#include <string>
#include <fstream>
using json = nlohmann::json;
int main()
{
    std::ifstream f("example.json");
    json data = json::parse(f);
    std::string name = data["name"];
    float pi = data["pi"];
    printf("name:%s pi:%f\n",name.c_str(), pi);
    return 0;
}