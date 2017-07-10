/** MessagePack Serializer / Deserializer Backend
 *  for ``MessagePack Specification'' please refer to [https://github.com/msgpack/msgpack/blob/master/spec.md]
 **/

#include "Variant.h"
#include "TypeInfo.h"
#include "Exceptions.h"
#include "backend/MessagePackBackend.h"

namespace SimpleRPC
{
namespace Backends
{
Variant MessagePackBackend::doParse(ByteSeq &data) const
{
    uint8_t ch;
    switch ((ch = data.nextBE<uint8_t>()))
    {
        /* positive fixint */
        case 0x00 ... 0x7f:
            return (int8_t)(ch & 0x7f);

        /* nil */
        case 0xc0:
            return Variant();

        /* (never used) */
        case 0xc1:
            throw Exceptions::DeserializerError("Reserved leading byte '\\xc1'");

        /* false */
        case 0xc2:
            return false;

        /* true */
        case 0xc3:
            return true;

        /* bin8/16/32 */
        case 0xc4:
        case 0xc5:
        case 0xc6:
            throw Exceptions::DeserializerError("\"Binary\" types are reserved for future purpose");

        /* ext8/16/32 */
        case 0xc7:
        case 0xc8:
        case 0xc9:
            throw Exceptions::DeserializerError("\"Extension\" types are reserved for future purpose");

        /* float */
        case 0xca:
            return data.nextBE<float>();

        /* double */
        case 0xcb:
            return data.nextBE<double>();

        /* uint8 */
        case 0xcc:
            return data.nextBE<uint8_t>();

        /* uint16 */
        case 0xcd:
            return data.nextBE<uint16_t>();

        /* uint32 */
        case 0xce:
            return data.nextBE<uint32_t>();

        /* uint64 */
        case 0xcf:
            return data.nextBE<uint64_t>();

        /* int8 */
        case 0xd0:
            return data.nextBE<int8_t>();

        /* int16 */
        case 0xd1:
            return data.nextBE<int16_t>();

        /* int32 */
        case 0xd2:
            return data.nextBE<int32_t>();

        /* int64 */
        case 0xd3:
            return data.nextBE<int64_t>();

        /* fixext1/2/4/8/16 */
        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
        case 0xd8:
            throw Exceptions::DeserializerError("\"Extension\" types are reserved for future purpose");

        /* str8/16/32 */
        case 0xd9:
        case 0xda:
        case 0xdb:

        /* fixstr */
        case 0xa0 ... 0xbf:
        {
            size_t n =
                ch == 0xd9 ? data.nextBE<uint8_t >() :  /* str8   */
                ch == 0xda ? data.nextBE<uint16_t>() :  /* str16  */
                ch == 0xdb ? data.nextBE<uint32_t>() :  /* str32  */
                static_cast<size_t>(ch & 0x1f);         /* fixstr */

            /* extract string from buffer */
            return std::string(data.consume(n), n);
        }

        /* array16/32 */
        case 0xdc:
        case 0xdd:

        /* fixarray */
        case 0x90 ... 0x9f:
        {
            size_t n =
                ch == 0xdc ? data.nextBE<uint16_t>() :  /* array16  */
                ch == 0xdd ? data.nextBE<uint32_t>() :  /* array32  */
                static_cast<size_t>(ch & 0x0f);         /* fixarray */

            /* make an array variant */
            Variant result(Type::TypeCode::Array);

            /* parse each element */
            while (n--)
                result.internalArray().push_back(std::make_shared<Variant>(doParse(data)));

            return result;
        }

        /* map16/32 */
        case 0xde:
        case 0xdf:

        /* fixmap */
        case 0x80 ... 0x8f:
        {
            size_t n =
                ch == 0xde ? data.nextBE<uint16_t>() :  /* map16  */
                ch == 0xdf ? data.nextBE<uint32_t>() :  /* map32  */
                static_cast<size_t>(ch & 0x0f);         /* fixmap */

            /* make an object variant */
            Variant result(Type::TypeCode::Object);

            while (n--)
            {
                Variant key   = doParse(data); /* odd elements in objects are keys of a map */
                Variant value = doParse(data); /* the next element of a key is its associated value */

                if (key.type() != Type::TypeCode::String)
                    throw Exceptions::DeserializerError("Keys of \"Object\" type must be strings");

                result.internalObject().emplace(
                    key.get<const std::string &>(),
                    std::make_shared<Variant>(std::move(value))
                );
            }

            return result;
        }

        /* negative fixint */
        case 0xe0 ... 0xff:
            return (int8_t)ch;

        default:
        {
            /* would NEVER happens */
            abort();
        }
    }
}

ByteSeq MessagePackBackend::doAssemble(Variant &object) const
{
    ByteSeq result;
    switch (object.type())
    {
        case Type::TypeCode::Void:
        {
            /* `void` is serialized as `Nil` */
            result.appendBE((uint8_t)0xc0);
            break;
        }

        case Type::TypeCode::Int8:
        {
            /* convert to `int8_t` */
            int8_t value = object.get<int8_t>();

            /* fixint (0 ~ 127) and negative fixint (-32 ~ 0) */
            if (value >= -32)
            {
                result.appendBE(value);
                break;
            }

            /* otherwise store as "int8" */
            result.appendBE((uint8_t)0xcc);
            result.appendBE(value);
            break;
        }

        case Type::TypeCode::Int16:
        {
            result.appendBE((uint8_t)0xd1);
            result.appendBE(object.get<int16_t>());
            break;
        }

        case Type::TypeCode::Int32:
        {
            result.appendBE((uint8_t)0xd2);
            result.appendBE(object.get<int32_t>());
            break;
        }

        case Type::TypeCode::Int64:
        {
            result.appendBE((uint8_t)0xd3);
            result.appendBE(object.get<int64_t>());
            break;
        }

        case Type::TypeCode::UInt8:
        {
            result.appendBE((uint8_t)0xcc);
            result.appendBE(object.get<uint8_t>());
            break;
        }

        case Type::TypeCode::UInt16:
        {
            result.appendBE((uint8_t)0xcd);
            result.appendBE(object.get<uint8_t>());
            break;
        }

        case Type::TypeCode::UInt32:
        {
            result.appendBE((uint8_t)0xce);
            result.appendBE(object.get<uint8_t>());
            break;
        }

        case Type::TypeCode::UInt64:
        {
            result.appendBE((uint8_t)0xcf);
            result.appendBE(object.get<uint8_t>());
            break;
        }

        case Type::TypeCode::Float:
        {
            result.appendBE((uint8_t)0xca);
            result.appendBE(object.get<float>());
            break;
        }

        case Type::TypeCode::Double:
        {
            result.appendBE((uint8_t)0xcb);
            result.appendBE(object.get<double>());
            break;
        }

        case Type::TypeCode::Boolean:
        {
            if (object.get<bool>())
                result.appendBE((uint8_t)0xc3);
            else
                result.appendBE((uint8_t)0xc2);

            break;
        }

        case Type::TypeCode::String:
        {
            /* get as reference to prevent copy */
            const std::string &s = object.get<const std::string &>();

            if (s.size() < 31)
            {
                /* fixstr */
                result.appendBE(static_cast<uint8_t>(0xa0 | s.size()));
            }
            else if (s.size() <= UINT8_MAX)
            {
                /* str8 */
                result.appendBE((uint8_t)0xd9);
                result.appendBE(static_cast<uint8_t>(s.size()));
            }
            else if (s.size() <= UINT16_MAX)
            {
                /* str16 */
                result.appendBE((uint8_t)0xda);
                result.appendBE(static_cast<uint16_t>(s.size()));
            }
            else if (s.size() <= UINT32_MAX)
            {
                /* str32 */
                result.appendBE((uint8_t)0xdb);
                result.appendBE(static_cast<uint32_t>(s.size()));
            }
            else
            {
                /* string is too long */
                throw Exceptions::SerializerError("String is too long : " + std::to_string(s.size()));
            }

            /* string content */
            result.append(s);
            break;
        }

        case Type::TypeCode::Array:
        {
            if (object.internalArray().size() <= 15)
            {
                /* fixarray */
                result.appendBE(static_cast<uint8_t>(0x90 | object.internalArray().size()));
            }
            else if (object.internalArray().size() <= UINT16_MAX)
            {
                /* array16 */
                result.appendBE((uint8_t)0xdc);
                result.appendBE(static_cast<uint16_t>(object.internalArray().size()));
            }
            else if (object.internalArray().size() <= UINT32_MAX)
            {
                /* array32 */
                result.appendBE((uint8_t)0xdd);
                result.appendBE(static_cast<uint32_t>(object.internalArray().size()));
            }
            else
            {
                /* array is too long */
                throw Exceptions::SerializerError("Array is too long : " + std::to_string(object.internalArray().size()));
            }

            /* serialize each object */
            for (const auto &item : object.internalArray())
                result.append(doAssemble(*item));

            break;
        }

        case Type::TypeCode::Object:
        {
            if (object.internalObject().size() <= 15)
            {
                /* fixmap */
                result.appendBE(static_cast<uint8_t>(0x80 | object.internalObject().size()));
            }
            else if (object.internalObject().size() <= UINT16_MAX)
            {
                /* map16 */
                result.appendBE((uint8_t)0xde);
                result.appendBE(static_cast<uint16_t>(object.internalObject().size()));
            }
            else if (object.internalObject().size() <= UINT32_MAX)
            {
                /* map32 */
                result.appendBE((uint8_t)0xdf);
                result.appendBE(static_cast<uint32_t>(object.internalObject().size()));
            }
            else
            {
                /* object is too large */
                throw Exceptions::SerializerError("Object is too large : " + std::to_string(object.internalObject().size()));
            }

            /* serialize each object */
            for (const auto &item : object.internalObject())
            {
                result.append(assemble(Variant(item.first)));
                result.append(doAssemble(*item.second));
            }

            break;
        }
    }

    return std::move(result);
}

/* register backend into registry */
defineBackend(MessagePackBackend)
}
}
