#ifndef GEL_TRIE_H
#define GEL_TRIE_H

#include <array>
#include <functional>
#include <string>

#include "gel/platform.h"

namespace gel::trie {
template <typename Value, const uword AlphabetSize>
struct Node {
  std::array<Node*, AlphabetSize> children{};
  bool epsilon = false;
  Value value{};
};

template <typename Value, const uword AlphabetSize>
static inline auto Insert(Node<Value, AlphabetSize>* root, const std::string& key, const Value& value) -> bool {
  ASSERT(root);
  auto current = root;
  for (const auto& c : key) {
    if (current->children.at(c) == nullptr)
      current->children.at(c) = new Node<Value, AlphabetSize>();
    current = current->children.at(c);
  }
  current->value = value;
  current->epsilon = true;
  return true;
}

template <typename Value, const uword AlphabetSize>
static inline auto SearchOrCreate(Node<Value, AlphabetSize>* root, const std::string& key, Value* result,
                                  const std::function<Value(const std::string& key)>& supplier) -> bool {
  ASSERT(root);
  auto current = root;
  for (const auto& c : key) {
    if (current->children.at(c) == nullptr)
      current->children.at(c) = new Node<Value, AlphabetSize>();
    current = current->children.at(c);
  }
  ASSERT(current);
  if (!current->epsilon) {
    current->value = supplier(key);
    current->epsilon = true;
    (*result) = current->value;
    return true;
  }
  ASSERT(current && current->epsilon);
  (*result) = current->value;
  return true;
}

template <typename Value, const uword AlphabetSize>
static inline auto Search(Node<Value, AlphabetSize>* root, const std::string& key, Value* result) -> bool {
  ASSERT(root);
  auto current = root;
  for (const auto& c : key) {
    if (current->children.at(c) == nullptr) {
      (*result) = (Value) nullptr;
      return false;
    }
    current = current->children.at(c);
  }
  if (!current || !current->epsilon) {
    (*result) = (Value) nullptr;
    return false;
  }
  ASSERT(current && current->epsilon);
  (*result) = current->value;
  return true;
}
}  // namespace gel::trie

#endif  // GEL_TRIE_H
