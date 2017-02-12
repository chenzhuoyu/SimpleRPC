#include <iostream>
#include "Serializable.h"

defineClass(Test,
    defineRequiredField(int, a),
    defineOptionalField(std::string, b)
)

int main()
{
    Test test;
    std::cout << "class: " << test.readableName() << std::endl;
    for (const auto &pair : test.meta().fields)
    {
        if (pair.second.isRequired())
            std::cout << pair.second.type().toString() << " " << pair.second.name() << ";" << std::endl;
        else
            std::cout << pair.second.type().toString() << " " << pair.second.name() << "; /* optional */" << std::endl;
    }
    return 0;
}
