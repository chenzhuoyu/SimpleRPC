#include <iostream>
#include "Backend.h"
#include "SimpleRPC.h"

struct Test final : public SimpleRPC::SerializableWrapper<Test>
{
    int n;
    int test(std::string &x, int y);
};
static SimpleRPC::Internal::Descriptor<Test> __register__ [[gnu::unused]] ({
    SimpleRPC::Internal::Descriptor<Test>::MemberData("n", ((Test *)nullptr)->n),
    SimpleRPC::Internal::Descriptor<Test>::MemberData("test", &Test::test),
});

int Test::test(std::string &x, int y)
{
    fprintf(stderr, "this is %p\n", this);
    fprintf(stderr, "x is %s\n", x.c_str());
    fprintf(stderr, "y is %d\n", y);
    x = "string from test";
    return 456123;
}

int main()
{
    /* class lookup, the leading "4" is because of C++ name mangling, for more details please Google it */
    const SimpleRPC::Serializable::Meta &meta = SimpleRPC::Registry::findClass("{4Test}");
    std::shared_ptr<Test> test(meta.newInstance<Test>());

    /* method lookup, the method name is it's signature */
    const std::shared_ptr<SimpleRPC::Method> &method1 = meta.methods().at("test(s&i)i");

    /* print method's name signature */
    fprintf(stderr, "%s::%s\n", meta.name().c_str(), method1->name().c_str());

    /* invoke method using reflection */
    SimpleRPC::Variant args = { "hello, world", 1234 };
    fprintf(stderr, "args-before: %s\n", args.toString().c_str());
    fprintf(stderr, "result: %s\n", method1->invoke(test.get(), args).toString().c_str());
    fprintf(stderr, "args-after: %s\n", args.toString().c_str());
    return 0;
}
