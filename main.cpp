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
static ::SimpleRPC::Descriptor<Test> __SimpleRPC_Test_Descriptor_DO_NOT_TOUCH_THIS_VARIABLE__ [[maybe_unused]] ({
    ::SimpleRPC::Descriptor<Test>::MemberData("a", ((Test*)nullptr)->a, true),
    ::SimpleRPC::Descriptor<Test>::MemberData("b", ((Test*)nullptr)->b, true),
    ::SimpleRPC::Descriptor<Test>::MemberData(&Test::test)
});

int Test::test(long x, std::string y)
{
    fprintf(stderr, "=====\n");
    fprintf(stderr, "%p %ld %s\n", this, x, y.c_str());
    fprintf(stderr, "=====\n");
    return 123;
}

int main()
{
    Test test;
    for (const auto &pair : test.meta().fields)
    {
        if (pair.second.isRequired())
            std::cerr << pair.second.type().toString() << " " << pair.second.name() << "; /* offset : " << pair.second.offset() << " */" << std::endl;
        else
            std::cerr << pair.second.type().toString() << " " << pair.second.name() << "; /* offset : " << pair.second.offset() << ", optional */" << std::endl;
    }

    return 0;
}
