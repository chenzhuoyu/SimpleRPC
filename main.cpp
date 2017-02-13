#include <iostream>
#include "Serializable.h"

//defineClass(Test,
//    defineField(int, a),
//    defineField(std::string, b),
//
//    defineMethod(int, test, long x, std::string y = std::string("Asdf"))
//)

struct Test : public ::SimpleRPC::Serializable {
    Test() { ::SimpleRPC::Serializable::setName(typeid(Test).name()); }
    int a;
    std::string b;
    int test(long x, std::string y);
};
static ::SimpleRPC::Descriptor<Test> __SimpleRPC_Test_Descriptor_DO_NOT_TOUCH_THIS_VARIABLE__({
    ::SimpleRPC::Descriptor<Test>::MemberData("a", ::SimpleRPC::resolve(((Test*)nullptr)->a), (size_t)(&(((Test*)nullptr)->a)), true),
    ::SimpleRPC::Descriptor<Test>::MemberData("b", ::SimpleRPC::resolve(((Test*)nullptr)->b), (size_t)(&(((Test*)nullptr)->b)), true),
    ::SimpleRPC::Descriptor<Test>::MemberData::makeFunction(&Test::test)
});

int Test::test(long x, std::string y)
{
    fprintf(stderr, "%p %ld %s\n", this, x, y.c_str());
    return 123;
}

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
