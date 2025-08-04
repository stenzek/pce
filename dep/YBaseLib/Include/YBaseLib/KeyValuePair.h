#pragma once

template<typename K, typename V>
struct KeyValuePair
{
  KeyValuePair() {}
  KeyValuePair(const K& rKey, const V& rValue) : Key(rKey), Value(rValue) {}
  KeyValuePair(const KeyValuePair<K, V>& c) : Key(c.Key), Value(c.Value) {}
  ~KeyValuePair() {}

  KeyValuePair& operator=(const KeyValuePair& copy)
  {
    Key = copy.Key;
    Value = copy.Value;
    return *this;
  }

  K Key;
  V Value;
};
