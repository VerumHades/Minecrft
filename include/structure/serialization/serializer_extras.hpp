#pragma once

#define SerializeFunction(type)   template <> bool Serializer::Serialize<type>(type& this_, ByteArray& array)
#define SerializeInstatiate(type) template bool Serializer::Serialize<type>(type&, ByteArray&);
#define DeserializeFunction(type) template <> bool Serializer::Deserialize<type>(type& this_, ByteArray& array)
#define DeserializeInstatiate(type) template bool Serializer::Deserialize<type>(type&, ByteArray&);


#define ResolvedOption(resolved_name, function, ...) \
    auto resolved_name##_option = array.function(__VA_ARGS__);\
    if(!resolved_name##_option) return false;\
    auto& resolved_name = resolved_name##_option.value();\
