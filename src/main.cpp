#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include "glzt.cpp"
#include "handler.cpp"

int main(int argc, char const *argv[])
{

    std::vector<std::string> args;

    for (size_t i = 0; i < argc; i++)
        args.push_back(argv[i]);

    Konsole k(args);
    k.start();

    return 0;
}
