/**
 * @file trie.cpp
 * @author Zihao Xu (xzhseh@gmail.com)
 * @copyright Copyright (c) 2023
 */

#include <trie.h>

template <class T>
auto Trie::Get(std::string_view key) const -> const T * {
  // If the root_ doesn't exist, return nullptr for calling a totally empty Trie
  if (root_ == nullptr) {
    return nullptr;
  }
  // Empty key case
  if (key.empty()) {
    auto return_node = dynamic_cast<const TrieNodeWithValue<T> *>(root_.get());
    // The root is not TrieNodeWithValue<T>
    if (return_node == nullptr) {
      return nullptr;
    }
    return return_node->value_.get();
  }
  // If the root_ doesn't contain the first char of the provided key, return nullptr
  if (root_->children_.find(key.at(0)) == root_->children_.end()) {
    return nullptr;
  }
  // Copy Constructor of the std::shared_ptr<>, instantiating the val of the to-be-traversed node
  std::shared_ptr<const TrieNode> curr{root_};

  curr = curr->children_.at(key.at(0));

  for (size_t i = 1; i < key.size(); ++i) {
    // Get the current char for subsequent searching
    const auto &cur_char = key.at(i);
    // Same check as before
    if (curr == nullptr) {
      return nullptr;
    }
    // There is no key-value pair exist for the next search char
    if (curr->children_.find(cur_char) == curr->children_.end()) {
      return nullptr;
    }
    // Move to next position of Trie
    curr = curr->children_.at(cur_char);
  }

  auto return_node = dynamic_cast<const TrieNodeWithValue<T> *>(curr.get());

  // Here implicitly dealing with the case when the node is not a TrieNodeWithValue<T>
  // Which will make dynamic_cast fail and return nullptr
  if (return_node == nullptr) {
    return nullptr;
  }
  // Get the raw pointer out of std::shared_ptr<T>
  return return_node->value_.get();
}

template <class T>
auto Trie::Put(std::string_view key, T value) const -> Trie {
  // Note that `T` might be a non-copyable type. Always use `std::move` when creating `shared_ptr` on that value.
  // i.e. The value passing in can be std::unique_ptr<int>, which can't be copied
  std::shared_ptr<TrieNode> new_root;
  // if non-exist..., and return the new Trie without changing anything
  // related to the original one, even if its barely empty...
  if (root_ == nullptr) {
    new_root = std::make_shared<TrieNode>();
  }
  // If the key is empty string, then set the value_ of root and return
  if (key.empty()) {
    std::shared_ptr<TrieNodeWithValue<T>> emp_root =
        std::make_shared<TrieNodeWithValue<T>>(root_->children_, std::make_shared<T>(std::move(value)));

    return Trie(emp_root);
  }
  // If the root exists, then create a new root with root_ instantiated using the Trie::Clone()
  // Note that this new_root has exact same children as root_, so further copying is needed
  if (root_ != nullptr) {
    new_root = std::shared_ptr<TrieNode>(root_->Clone().release());
  }
  // Then make a traverse node just as we did in Trie::Get()
  std::shared_ptr<TrieNode> curr{new_root};

  for (size_t i = 0; i < key.size(); ++i) {
    // Get the current char from key
    const auto &cur_char = key.at(i);
    // If the next child is nullptr, which means it doesn't exist
    if (curr->children_.find(cur_char) == curr->children_.end() && i != key.size() - 1) {
      // Then first create a new node from the 'curr' node,
      // remember we can't change anything from the original Trie
      // std::shared_ptr<const TrieNode> tmp_node = curr->Clone();
      // Create a new node
      std::shared_ptr<TrieNode> tmp_node = std::make_shared<TrieNode>();
      // Connect parent node(curr) with new child node
      curr->children_[cur_char] = tmp_node;
      curr = tmp_node;
      continue;
    }

    if (curr->children_.find(cur_char) == curr->children_.end() && i == key.size() - 1) {
      // This represents the case of new value node
      std::shared_ptr<const TrieNodeWithValue<T>> tmp_val_node =
          std::make_shared<const TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)));

      // Make connection
      curr->children_[cur_char] = tmp_val_node;
      break;
    }

    if (i == key.size() - 1) {
      // This represents the case of traversing to the correct position
      // No matter what cases, should we create a new TrieNodeWithValue<T>
      std::shared_ptr<TrieNodeWithValue<T>> tmp_val_node;

      if (curr->children_[cur_char] != nullptr) {
        // There is still other children
        tmp_val_node = std::make_shared<TrieNodeWithValue<T>>(curr->children_[cur_char]->children_,
                                                              std::make_shared<T>(std::move(value)));
      } else {
        // There is no available children
        tmp_val_node = std::make_shared<TrieNodeWithValue<T>>(std::make_shared<T>(std::move(value)));
      }

      curr->children_[cur_char] = tmp_val_node;

      break;
    }

    // Then the following node is neither nullptr nor the end of search
    // Just copy the corresponding TrieNode and continue the process
    std::shared_ptr<TrieNode> tmp_val_node = std::shared_ptr<TrieNode>(curr->children_[cur_char]->Clone().release());

    // Set the connection
    curr->children_[cur_char] = tmp_val_node;
    // Continue the process
    curr = tmp_val_node;
  }

  return Trie(new_root);
}

auto Trie::Remove(std::string_view key) const -> Trie {
  // Dealing with special cases pertaining with root_ and empty key.
  // Note: Delete node from empty Trie is strictly prohibited
  // In such case, just return *this, which is an empty trie also.
  if ((key.empty() && !root_->is_value_node_) || (root_ == nullptr)) {
    return *this;
  }
  if (key.empty() && root_->is_value_node_) {
    auto new_root = std::shared_ptr<TrieNode>(root_->Clone().release());
    return Trie(new_root);
  }

  auto new_root = std::shared_ptr<TrieNode>(root_->Clone().release());
  auto curr{new_root};

  for (size_t i = 0; i < key.size(); ++i) {
    const auto &cur_char = key.at(i);

    if (curr->children_.find(cur_char) == curr->children_.end()) {
      // Remove doesn't actually remove anything, no key-value pair found
      // Thus, just return itself
      return *this;
    }

    auto cur_child_node = std::shared_ptr<TrieNode>(curr->children_[cur_char]->Clone().release());

    if (i == key.size() - 1) {
      // Will automatically release every pre-copied nodes
      // after the function ends
      if (!cur_child_node->is_value_node_) {
        return *this;
      }
      // No children, then remove the node
      // By making no connection with the parent node(curr)
      if (cur_child_node->children_.empty()) {
        // Explicitly making parent node(curr)
        // connection to be nullptr
        curr->children_[cur_char] = nullptr;
        break;
      }
      // Otherwise, make the current node back to TrieNode
      // We need to explicitly make a copy
      cur_child_node = std::make_shared<TrieNode>(cur_child_node->children_);
      curr->children_[cur_char] = cur_child_node;
      break;
    }

    curr->children_[cur_char] = cur_child_node;
    curr = cur_child_node;
  }

  return Trie(new_root);
}

// Below are explicit instantiation of template functions.
//
// Generally people would write the implementation of template classes and functions in the header file. However, we
// separate the implementation into a .cpp file to make things clearer. In order to make the compiler know the
// implementation of the template functions, we need to explicitly instantiate them here, so that they can be picked up
// by the linker.

template auto Trie::Put(std::string_view key, uint32_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint32_t *;

template auto Trie::Put(std::string_view key, uint64_t value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const uint64_t *;

template auto Trie::Put(std::string_view key, std::string value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const std::string *;

using Integer = std::unique_ptr<uint32_t>;

template auto Trie::Put(std::string_view key, Integer value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const Integer *;

template auto Trie::Put(std::string_view key, MoveBlocked value) const -> Trie;
template auto Trie::Get(std::string_view key) const -> const MoveBlocked *;
