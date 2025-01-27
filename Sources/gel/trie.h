#ifndef GEL_TRIE_H
#define GEL_TRIE_H

#include <array>
#include <functional>
#include <string>

#include "gel/platform.h"

namespace gel::trie {
template <typename K, typename V, const uword AlphabetSize>
struct Node {
  std::array<Node*, AlphabetSize> children{};
  bool epsilon = false;
  V value{};
};

template <typename K, typename V, const uword AlphabetSize>
static inline auto Insert(Node<K, V, AlphabetSize>* root, const K& key, const V& value) -> Node<K, V, AlphabetSize>* {
  ASSERT(root);
  auto current = root;
  for (const auto& c : key) {
    if (current->children.at(c) == nullptr)
      current->children.at(c) = new Node<K, V, AlphabetSize>();
    current = current->children.at(c);
  }
  current->value = value;
  current->epsilon = true;
  return current;
}

template <typename K, typename V, const uword AlphabetSize>
static inline auto SearchOrCreate(Node<K, V, AlphabetSize>* root, const K& key, V* result,
                                  const std::function<V(const std::string& key)>& supplier) -> bool {
  ASSERT(root);
  auto current = root;
  for (const auto& c : key) {
    if (current->children.at(c) == nullptr)
      current->children.at(c) = new Node<K, V, AlphabetSize>();
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

template <typename K, typename V, const uword AlphabetSize>
static inline auto Search(Node<K, V, AlphabetSize>* root, const K& key, V* result) -> bool {
  ASSERT(root);
  auto current = root;
  for (const auto& c : key) {
    if (current->children.at(c) == nullptr) {
      (*result) = (V) nullptr;
      return false;
    }
    current = current->children.at(c);
  }
  if (!current || !current->epsilon) {
    (*result) = (V) nullptr;
    return false;
  }
  ASSERT(current && current->epsilon);
  (*result) = current->value;
  return true;
}
}  // namespace gel::trie

#endif  // GEL_TRIE_H
