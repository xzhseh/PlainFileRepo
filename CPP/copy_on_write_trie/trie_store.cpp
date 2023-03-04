/**
 * @file trie_store.cpp
 * @author Zihao Xu (xzhseh@gmail.com)
 * @copyright Copyright (c) 2023
 */

#include <trie_store.h>

template <class T>
auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<T>> {
  root_lock_.lock();
  Trie curr_root = root_;
  root_lock_.unlock();

  auto result = curr_root.Get<T>(key);
  if (result == nullptr) {
    return std::nullopt;
  }

  return ValueGuard<T>(curr_root, *result);
}

template <class T>
void TrieStore::Put(std::string_view key, T value) {
  write_lock_.lock();

  root_lock_.lock();
  Trie curr_root = root_;
  root_lock_.unlock();

  curr_root = curr_root.Put<T>(key, std::move(value));

  root_lock_.lock();
  root_ = curr_root;
  root_lock_.unlock();

  write_lock_.unlock();
}

void TrieStore::Remove(std::string_view key) {
  write_lock_.lock();

  root_lock_.lock();
  Trie curr_root = root_;
  root_lock_.unlock();

  curr_root = curr_root.Remove(key);

  root_lock_.lock();
  root_ = curr_root;
  root_lock_.unlock();

  write_lock_.unlock();
}

// Below are explicit instantiation of template functions.

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<uint32_t>>;
template void TrieStore::Put(std::string_view key, uint32_t value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<std::string>>;
template void TrieStore::Put(std::string_view key, std::string value);

using Integer = std::unique_ptr<uint32_t>;

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<Integer>>;
template void TrieStore::Put(std::string_view key, Integer value);

template auto TrieStore::Get(std::string_view key) -> std::optional<ValueGuard<MoveBlocked>>;
template void TrieStore::Put(std::string_view key, MoveBlocked value);