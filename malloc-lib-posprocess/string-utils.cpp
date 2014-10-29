#include "string-utils.h"
#include <sstream>

std::vector<std::string> splitString(std::string str, char character)
{
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> subStrVector;
    while (std::getline(ss, item, character))
    {
        if(!item.empty())
            subStrVector.push_back(item);
    }
    return subStrVector;
}
