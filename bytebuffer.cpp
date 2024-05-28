#include "bytebuffer.h"

std::ostream& operator<<(std::ostream& os, const ByteBuffer& bb)
{
    return os << (bb.empty() ? "" : std::string(bb));
}

const ByteBuffer ByteBuffer::EMPTY = ByteBuffer();
