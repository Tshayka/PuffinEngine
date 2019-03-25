
#include <iostream>

#include "puffinEngine/src/PuffinEngine.hpp"

int main(int argc, char* argv[]){
	
	PuffinEngine engine;

    try {
		engine.Run();

        // char l;
        // std::cin >> l;
        // std::cout << engine.isUppercase(l) << std::endl;
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

    return EXIT_SUCCESS;
}
