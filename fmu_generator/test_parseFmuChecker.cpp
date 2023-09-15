#include <iostream>
#include <fstream>
#include <regex>
#include <string>

int main() {
    std::ifstream file(FMUCHECKER_LOG);
    if (!file.is_open()) {
        std::cerr << "Error opening file." << std::endl;
        return 1;
    }

    std::regex errorRegex(R"(\[ERROR\](.*))");
    std::regex warningRegex(R"(\[WARNING\](.*))");
    std::string line;

    bool hasError = false;
    while (std::getline(file, line)) {
        std::smatch match;
        if (std::regex_search(line, match, errorRegex)) {
            std::cout << "ERROR: " << match[1] << std::endl;
            hasError = true;
        }
        else if (std::regex_search(line, match, warningRegex)) {
            std::cout << "WARNING: " << match[1] << std::endl;
        }
    }

    file.close();
    return hasError ? 1 : 0;
}