//
// Created by Caio Ferreira Cardoso on 18/06/26.
//

#include <map>
#include <sstream>
#include <string>

std::map<std::string, std::string> parsePayload(const std::string &payload) {
    std::map<std::string, std::string> result;

    std::istringstream stream(payload);
    std::string item;

    while (std::getline(stream, item, ';')) {
        std::size_t separator = item.find('=');

        if (separator == std::string::npos) {
            continue;
        }

        std::string key = item.substr(0, separator);
        std::string value = item.substr(separator + 1);

        result[key] = value;
    }

    return result;
}
