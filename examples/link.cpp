#include <iostream>

extern "C" {
int mast();
}

int main() {
    std::cout << mast() << std::endl;
}