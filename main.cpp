#include <iostream>
#include "Serializable.h"

defineClass(Test,
    defineField(int, a),
    defineField(std::string, b),

    declareMethod(int, test, long x, std::string y)
)

int Test::test(long x, std::string y)
{
    fprintf(stderr, "=====\n");
    fprintf(stderr, "this: %p, this->a: %d, x: %ld, y: %s, this->b: %s\n", this, a, x, y.c_str(), b.c_str());
    fprintf(stderr, "=====\n");
    return 456123;
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
    const std::shared_ptr<SimpleRPC::Method> &method = meta.methods().at("M4TestFilNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEE");

    /* print method's return type and name signature */
    fprintf(stderr, "%s %s::%s(...)\n", method->result().toString().c_str(), meta.name().c_str(), method->name().c_str());

    /* invoke method using reflection */
    fprintf(stderr, "result: %d\n", method->invoke(test.get(), SimpleRPC::Variant::array(123, "hello, world")).as<int>());
    return 0;
}
