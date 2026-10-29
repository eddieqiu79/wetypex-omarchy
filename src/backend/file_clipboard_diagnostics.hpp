#pragma once
#include "../common/json.hpp"
#include <cstdint>
#include <stdexcept>
#include <vector>

// Bounded, read-only protobuf structure inspection. Never serialize values or
// descend into clipboard bodies, names, URLs, extra-info JSON, or credentials.
namespace file_clipboard_diagnostics {
struct Bytes {
  const unsigned char *data;
  size_t length;
  Bytes(const std::vector<unsigned char> &v) : data(v.data()), length(v.size()) {}
  Bytes(const unsigned char *data, size_t length) : data(data), length(length) {}
  size_t size() const { return length; }
  unsigned char operator[](size_t i) const { return data[i]; }
  Bytes subspan(size_t offset, size_t count) const {
    if (offset > length || count > length - offset)
      throw std::out_of_range("protobuf metadata slice");
    return Bytes(offset ? data + offset : data, count);
  }
  Bytes first(size_t count) const { return subspan(0, count); }
};
struct Field {
  uint32_t number;
  unsigned wire;
  size_t offset, bytes;
  uint64_t scalar;
};
struct Message {
  std::vector<Field> fields;
  bool valid = false;
};
inline bool varint(Bytes input, size_t &offset, uint64_t &value) {
  value = 0;
  for (unsigned i = 0; i < 10; ++i) {
    if (offset == input.size())
      return false;
    const auto byte = input[offset++];
    if (i == 9 && byte > 1)
      return false;
    value |= uint64_t(byte & 127) << (7 * i);
    if (!(byte & 128))
      return true;
  }
  return false;
}
inline Message parse(Bytes input) {
  Message result;
  if (input.size() > 16 * 1024 * 1024)
    return result;
  size_t offset = 0;
  while (offset < input.size()) {
    if (result.fields.size() == 4096)
      return result;
    uint64_t tag = 0, value = 0;
    if (!varint(input, offset, tag) || !(tag >> 3) ||
        (tag >> 3) > 0x1fffffff)
      return result;
    const unsigned wire = tag & 7;
    size_t start = offset, count = 0;
    if (wire == 0) {
      if (!varint(input, offset, value))
        return result;
      count = offset - start;
    } else if (wire == 2) {
      if (!varint(input, offset, value) || value > input.size() - offset)
        return result;
      start = offset;
      count = size_t(value);
      offset += count;
      value = 0;
    } else if (wire == 1 || wire == 5) {
      count = wire == 1 ? 8 : 4;
      if (count > input.size() - offset)
        return result;
      offset += count;
    } else {
      return result; // Obsolete protobuf groups are deliberately unsupported.
    }
    result.fields.push_back({uint32_t(tag >> 3), wire, start, count, value});
  }
  result.valid = true;
  return result;
}
inline uint64_t scalar(const Message &message, unsigned number) {
  // Protobuf's last occurrence wins. Used only for explicitly whitelisted
  // type/platform fields or comparisons that do not leave this function.
  uint64_t value = 0;
  for (const auto &field : message.fields)
    if (field.number == number && field.wire == 0)
      value = field.scalar;
  return value;
}
inline wire::Json shape(const Message &message) {
  auto result = wire::object();
  wire::put(result.get(), "valid", message.valid);
  auto fields = wire::Json(json_object_new_array());
  size_t count = 0;
  for (const auto &field : message.fields) {
    if (++count > 256)
      break;
    auto item = wire::object();
    wire::put(item.get(), "field", int64_t(field.number));
    wire::put(item.get(), "wire", int64_t(field.wire));
    wire::put(item.get(), "bytes", int64_t(field.bytes));
    json_object_array_add(fields.get(), item.release());
  }
  wire::put(result.get(), "fields_limited", message.fields.size() > 256);
  json_object_object_add(result.get(), "fields", fields.release());
  return result;
}
inline wire::Json inspect(Bytes response, uint64_t selfUin) {
  const auto envelope = parse(response);
  auto result = shape(envelope);
  auto groups = wire::Json(json_object_new_array());
  bool limited = false;
  size_t groupCount = 0, clipCount = 0;
  for (const auto &field : envelope.fields) {
    if (field.number != 1 || field.wire != 2)
      continue;
    if (++groupCount > 64) {
      limited = true;
      break;
    }
    const auto data = response.subspan(field.offset, field.bytes);
    const auto group = parse(data);
    auto item = shape(group);
    auto devices = wire::Json(json_object_new_array());
    struct Peer { uint64_t uin, platform; };
    std::vector<Peer> peers;
    for (const auto &boundField : group.fields) {
      if (boundField.number != 2 || boundField.wire != 2)
        continue;
      if (peers.size() >= 256) {
        limited = true;
        break;
      }
      const auto boundData = data.subspan(boundField.offset, boundField.bytes);
      const auto bound = parse(boundData);
      if (!bound.valid)
        continue;
      for (const auto &infoField : bound.fields) {
        if (infoField.number != 2 || infoField.wire != 2)
          continue;
        if (peers.size() >= 256) {
          limited = true;
          break;
        }
        const auto info = parse(boundData.subspan(infoField.offset, infoField.bytes));
        if (!info.valid)
          continue;
        const auto platform = scalar(info, 2);
        const auto uin = scalar(bound, 1);
        peers.push_back({uin, platform});
        auto device = wire::object();
        wire::put(device.get(), "platform", int64_t(platform <= 5 ? platform : 0));
        wire::put(device.get(), "is_self", uin != 0 && uin == selfUin);
        wire::put(device.get(), "supports_file_transfer", scalar(info, 5) == 1);
        json_object_array_add(devices.get(), device.release());
      }
    }
    auto clips = wire::Json(json_object_new_array());
    auto invites = wire::Json(json_object_new_array());
    for (const auto &child : group.fields) {
      if (child.wire != 2 || (child.number != 3 && child.number != 7))
        continue;
      if (++clipCount > 512) {
        limited = true;
        break;
      }
      const auto message = parse(data.subspan(child.offset, child.bytes));
      auto meta = shape(message);
      if (child.number == 3) {
        const auto type = scalar(message, 2);
        bool hasType = false;
        for (const auto &entry : message.fields)
          hasType |= entry.number == 2 && entry.wire == 0;
        if (hasType && type <= 65535)
          wire::put(meta.get(), "type_number", int64_t(type));
        const bool known = type == 1 || type == 2 || type == 5 || type == 6 || type == 100;
        wire::put(meta.get(), "type_known", known);
        if (known)
          wire::put(meta.get(), "type", int64_t(type));
        const auto source = scalar(message, 7);
        uint64_t platform = 0;
        bool member = false;
        for (const auto &peer : peers)
          if (source && source == peer.uin) {
            member = true;
            platform = peer.platform;
          }
        wire::put(meta.get(), "source_is_self", source != 0 && source == selfUin);
        wire::put(meta.get(), "source_is_group_member", member);
        wire::put(meta.get(), "source_platform", int64_t(platform <= 5 ? platform : 0));
        json_object_array_add(clips.get(), meta.release());
      } else {
        // Invitation code, name, sender, timestamps and capabilities stay opaque.
        json_object_array_add(invites.get(), meta.release());
      }
    }
    json_object_object_add(item.get(), "devices", devices.release());
    json_object_object_add(item.get(), "clips", clips.release());
    json_object_object_add(item.get(), "invites", invites.release());
    json_object_array_add(groups.get(), item.release());
  }
  wire::put(result.get(), "limited", limited);
  json_object_object_add(result.get(), "groups", groups.release());
  return result;
}
} // namespace file_clipboard_diagnostics
