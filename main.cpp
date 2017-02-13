#include <iostream>
#include "Serializable.h"

defineClass(Test,
    defineField(int, a),
    defineField(std::string, b),

    defineMethod(int, test, defineArg(long, x), defineArg(std::string, y, "asdf"))
)

int main()
{
    Test test;
    std::cout << "class: " << test.readableName() << ", a: " << test.a << ", b: " << test.b << std::endl;
    test.a = 100;
    test.b = "asdf";
    std::cout << "a: " << test.a << ", b: " << test.b << std::endl;
    std::cout << "a(ref): " << test.meta().fields.at("a").data<int>(&test) << ", b(ref): " << test.meta().fields.at("b").data<std::string>(&test) << std::endl;
    test.meta().fields.at("a").data<int>(&test) = 200;
    test.meta().fields.at("b").data<std::string>(&test) = "hello, world";
    std::cout << "a: " << test.a << ", b: " << test.b << std::endl;
    std::cout << "a(ref): " << test.meta().fields.at("a").data<int>(&test) << ", b(ref): " << test.meta().fields.at("b").data<std::string>(&test) << std::endl;
    for (const auto &pair : test.meta().fields)
    {
        if (pair.second.isRequired())
            std::cout << pair.second.type().toString() << " " << pair.second.name() << "; /* offset : " << pair.second.offset() << " */" << std::endl;
        else
            std::cout << pair.second.type().toString() << " " << pair.second.name() << "; /* offset : " << pair.second.offset() << ", optional */" << std::endl;
    }

    return 0;
}
