#include <map>
#include <iostream>
#include "SimpleRPC.h"
#include "network/LocalCallSite.h"

typedef std::unordered_map<int, std::string> TestMap;

defineClass(Test,
    defineField(int, n),
    defineField(TestMap, m),
    declareMethod(int, test, (std::vector<int> &, Test &, const std::string &, int &))
);

int Test::test(std::vector<int> &x, Test &y, const std::string &z, int &w)
{
    fprintf(stderr, "this is %p\n", this);
    fprintf(stderr, "z is %s\n", z.c_str());
    fprintf(stderr, "w is %d\n", w);
    fprintf(stderr, "y.n is %d\n", y.n);
    fprintf(stderr, "y.m is %s\n", SimpleRPC::Variant(y.m).toString().c_str());
    fprintf(stderr, "array is %s\n", SimpleRPC::Variant(x).toString().c_str());
    for (auto &n : x)
        n *= 10;
    fprintf(stderr, "now array is %s\n", SimpleRPC::Variant(x).toString().c_str());
    y.n = 666;
    y.m[123] = "hahahahahhaha";
    w = 777;
    return 12345;
}

int main()
{
    Test::Proxy test(new SimpleRPC::Network::LocalCallSite);
    std::vector<int> x = {1, 2, 3, 4};
    Test y;
    int w = 999;
    y.n = 12345;
    y.m.emplace(123, "hello, world");
    fprintf(stderr, "%s\n", SimpleRPC::Variant(test.test(x, y, "asd", w)).toString().c_str());
    fprintf(stderr, "%lu %d\n", x.size(), w);
    fprintf(stderr, "after array is %s\n", SimpleRPC::Variant(x).toString().c_str());
    fprintf(stderr, "after y is %s\n", SimpleRPC::Variant(y).toString().c_str());
    return 0;
}
