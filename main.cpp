#include <iostream>
#include "Serializable.h"

defineClass(Foo,
    defineField(std::vector<std::string>, x),
    defineLiteral(Foo(const std::vector<std::string> &v) : x(v) {})
)

defineClass(Test,
    defineField(int, a),
    defineField(std::string, b),
    defineField(std::vector<Foo>, c),

    declareMethod(int, test, long x, std::string y),
    declareMethod(int, test2, long x, std::string y, std::vector<int> p, Foo q)
)

int Test::test(long x, std::string y)
{
    fprintf(stderr, "=====\n");
    fprintf(stderr, "test : this: %p, this->a: %d, x: %ld, y: %s, this->b: %s\n", this, a, x, y.c_str(), b.c_str());
    for (const auto &p : c)
        for (const auto &q : p.x)
            fprintf(stderr, "c[].x: %s\n", q.c_str());
    fprintf(stderr, "=====\n");
    Foo foo1({ "response", "text" });
    Foo foo2({ "from", "test" });
    c.push_back(foo1);
    c.push_back(foo2);
    return 456123;
}

int Test::test2(long x, std::string y, std::vector<int> p, Foo q)
{
    fprintf(stderr, "xxxxx\n");
    fprintf(stderr, "test2 : this: %p, this->a: %d, x: %ld, y: %s, this->b: %s\n", this, a, x, y.c_str(), b.c_str());
    fprintf(stderr, "-: %ld\n", p.size());
    for (const auto &n : p)
        fprintf(stderr, "%d\n", n);
    fprintf(stderr, "-\n");
    for (const auto &n : q.x)
        fprintf(stderr, "Foo::x = %s\n", n.c_str());
    fprintf(stderr, "xxxxx\n");
    return 666666;
}

int main()
{
    /* class lookup, the leading "4" is because of C++ name mangling, for more details please Google it */
    const SimpleRPC::Serializable::Meta &meta = SimpleRPC::Registry::findClass("{4Test}");

    /* instaniate using reflection */
    std::shared_ptr<Test> test(meta.newInstance<Test>());

    /* set field using deserialization */
    test->deserialize(SimpleRPC::Variant::object({
        { "a", 156814 },
        { "b", "test deserialze" },
        { "c", SimpleRPC::Variant::array(
            SimpleRPC::Variant::object({{ "x", SimpleRPC::Variant::array("foo", "bar") }}),
            SimpleRPC::Variant::object({{ "x", SimpleRPC::Variant::array("baz", "fap") }})
        )},
    }));

    /* method lookup, the method name is it's signature */
    const std::shared_ptr<SimpleRPC::Method> &method1 = meta.methods().at("test(qs)i");

    /* print method's name signature */
    fprintf(stderr, "%s::%s\n", meta.name().c_str(), method1->name().c_str());

    /* invoke method using reflection */
    fprintf(stderr, "result: %d\n", method1->invoke(test.get(), SimpleRPC::Variant::array((int64_t)123, "hello, world")).get<int>());

    /* test serialize */
    fprintf(stderr, "serialize: %s\n", test->serialize().toString().c_str());

    /* method lookup, the method name is it's signature */
    const std::shared_ptr<SimpleRPC::Method> &method2 = meta.methods().at("test2(qs[i]{3Foo})i");

    /* print method's name signature */
    fprintf(stderr, "%s::%s\n", meta.name().c_str(), method2->name().c_str());

    /* invoke method using reflection */
    fprintf(stderr, "result: %d\n", method2->invoke(test.get(),
        SimpleRPC::Variant::array(
            (int64_t)999,
            "sdfdfg",
            SimpleRPC::Variant::array(1, 2),
            SimpleRPC::Variant::object({
                { "x", SimpleRPC::Variant::array("hello", "world") }
            })
        )
    ).get<int>());
    return 0;
}
