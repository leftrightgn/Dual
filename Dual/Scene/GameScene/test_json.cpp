#include <iostream>
#include <string>
#include ""../../../External/Engine/Common/json.hpp""

int main()
{
    nlohmann::json data;
    float x = 1.0f, y = 2.0f, z = 3.0f;
    data[""Position""] = { x, y, z };
    std::cout << data.dump() << std::endl;
    return 0;
}
