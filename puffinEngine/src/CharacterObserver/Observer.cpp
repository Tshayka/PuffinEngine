#include <string>

class Observer {
    public:
    virtual void Update() = 0;
    std::string description;
    unsigned int id = 1;
    bool finished = false;
};
