#include "internal.hpp"

namespace rosbags::codec {

// Generated message codecs use the same checked cursor semantics as the container readers.
// Keeping this translation unit in the base library gives generated sources a stable target.
void ensure_payload(ByteView payload, std::size_t minimum) {
  if (payload.size < minimum) throw DecodeError("serialized message is truncated");
}

}  // namespace rosbags::codec
