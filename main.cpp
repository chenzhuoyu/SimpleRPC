#include <iostream>
#include "Backend.h"
#include "SimpleRPC.h"

defineClass(Test,
    defineField(int, n),
    declareMethod(void, test, std::vector<int> &x, Test &y, const std::string &z, int w)
);

void Test::test(std::vector<int> &x, Test &y, const std::string &z, int w)
{
    fprintf(stderr, "this is %p\n", this);
    fprintf(stderr, "w is %d\n", w);
    fprintf(stderr, "array is %s\n", SimpleRPC::Variant(x).toString().c_str());
    for (auto &n : x)
        n *= 10;
    fprintf(stderr, "now array is %s\n", SimpleRPC::Variant(x).toString().c_str());
    y.n = 666;
}

int main()
{
    /* class lookup, the leading "4" is because of C++ name mangling, for more details please Google it */
    const SimpleRPC::Serializable::Meta &meta = SimpleRPC::Registry::findClass("{4Test}");
    std::shared_ptr<Test> test(meta.newInstance<Test>());

    /* method lookup, the method name is it's signature */
    const std::shared_ptr<SimpleRPC::Method> &method = meta.methods().at("test([i]&{4Test}&si)v");

    /* print method's name signature */
    fprintf(stderr, "%s::%s\n", meta.name().c_str(), method->name().c_str());

    /* invoke method using reflection */
    SimpleRPC::Variant args = { {1, 2, 3}, SimpleRPC::Variant::object({{ "n", 123 }}), "asd", 555 };
    fprintf(stderr, "args-before: %s\n", args.toString().c_str());
    fprintf(stderr, "result: %s\n", method->invoke(test.get(), args).toString().c_str());
    fprintf(stderr, "args-after: %s\n", args.toString().c_str());
    return 0;
}
