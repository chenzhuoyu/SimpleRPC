#include <iostream>
#include "Serializable.h"

defineClass(Test,
    defineRequiredField(int, a),
    defineOptionalField(std::string, b)
)

int main()
{
    SimpleRPC::Struct::Meta meta = SimpleRPC::Registry::findClass(typeid(Test).name());
    for (const auto &pair : meta.fields)
    {
        if (pair.second.isRequired())
            std::cout << pair.second.type().toString() << " " << pair.second.name() << ";" << std::endl;
        else
            std::cout << pair.second.type().toString() << " " << pair.second.name() << "; /* optional */" << std::endl;
    }
    return 0;
}
