#include <iostream>
#include "Serializable.h"

defineClass(Test,
    defineField(int, a),
    defineField(std::string, b),

    declareMethod(int, test, long x, std::string y),
    declareMethod(int, test2, long x, std::string y)
)

int Test::test(long x, std::string y)
{
    fprintf(stderr, "=====\n");
    fprintf(stderr, "test : this: %p, this->a: %d, x: %ld, y: %s, this->b: %s\n", this, a, x, y.c_str(), b.c_str());
    fprintf(stderr, "=====\n");
    return 456123;
}

int Test::test2(long x, std::string y)
{
    fprintf(stderr, "xxxxx\n");
    fprintf(stderr, "test2 : this: %p, this->a: %d, x: %ld, y: %s, this->b: %s\n", this, a, x, y.c_str(), b.c_str());
    fprintf(stderr, "xxxxx\n");
    return 666666;
}

int main()
{
    /* class lookup, the leading "4" is because of C++ name mangling, for more details please Google it */
    const SimpleRPC::Serializable::Meta &meta = SimpleRPC::Registry::findClass("4Test");

    /* instaniate using reflection */
    std::shared_ptr<Test> test(meta.newInstance<Test>());

    /* set field using reflection */
    meta.fields().at("a")->data<int>(test.get()) = 789;
    meta.fields().at("b")->data<std::string>(test.get()) = "test-reflect";

    /* method lookup, that mysterious method name is it's signature, please STFG (Search The Fucking Google) for more details */
    const std::shared_ptr<SimpleRPC::Method> &method1 = meta.methods().at(std::string("test::") + typeid(&Test::test).name());

    /* print method's return type and name signature */
    fprintf(stderr, "%s %s::%s(...)\n", method1->result().toString().c_str(), meta.name().c_str(), method1->name().c_str());

    /* invoke method using reflection */
    fprintf(stderr, "result: %d\n", method1->invoke(test.get(), SimpleRPC::Variant::array((int64_t)123, "hello, world")).get<int>());

    /* method lookup, that mysterious method name is it's signature, please STFG (Search The Fucking Google) for more details */
    const std::shared_ptr<SimpleRPC::Method> &method2 = meta.methods().at(std::string("test2::") + typeid(&Test::test).name());

    /* print method's return type and name signature */
    fprintf(stderr, "%s %s::%s(...)\n", method2->result().toString().c_str(), meta.name().c_str(), method1->name().c_str());

    /* invoke method using reflection */
    fprintf(stderr, "result: %d\n", method2->invoke(test.get(), SimpleRPC::Variant::array((int64_t)999, "sdfdfg")).get<int>());
    return 0;
}
