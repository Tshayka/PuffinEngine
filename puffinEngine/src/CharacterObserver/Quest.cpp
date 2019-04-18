#include <iostream>

#include "../ErrorCheck.hpp"
#include "../Character.hpp"

template <typename T>
class Quest: public Observer {
    public:
    Quest(std::shared_ptr<Character> mainCharacter, T amount, std::string description, T& observedValue) {

        this->mainCharacter = mainCharacter;
        this->amount = amount;
        this->description = description;
        this->observedValue = &observedValue;

        mainCharacter->AttachObservator(this);
    }

    ~Quest(){std::cout << "Observer object destroyed\n";};
    
    void Update() {
        if(*observedValue >= amount) {
            std::cout << "You have reached: " << description << '\n';
            mainCharacter->DettachObservator(this);
        }
    }

    private:
    T amount;
    T* observedValue;
    std::shared_ptr<Character> mainCharacter;
};