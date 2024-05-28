#include "keyvalue.h"

std::ostream& operator<<(std::ostream& os, const KeyValue& kv)
{
    return os << std::string(kv.key) << '/' << std::string(kv.value);
}

const KeyValue KeyValue::EMPTY = KeyValue(ByteBuffer::EMPTY, ByteBuffer::EMPTY);
