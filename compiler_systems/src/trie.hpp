#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

class ArenaTrie {
  struct Node {
    bool is_end_of_word = false;
    std::array<Node *, 26> children{};
  };

public:
  ArenaTrie() { nodes.push_back(std::make_unique<Node>()); };

  void insert(const std::string &word) {
    auto current_node_ptr = nodes.front().get();
    for (const auto character : word) {
      auto &child_node_ptr = current_node_ptr->children[character - 'a'];
      if (child_node_ptr == nullptr) {
        nodes.push_back(std::make_unique<Node>());
        child_node_ptr = nodes.back().get();
      }
      current_node_ptr = child_node_ptr;
    }
    current_node_ptr->is_end_of_word = true;
  }

  bool search(const std::string &word) const {
    const Node *final_node = find_last_node(word);
    if (final_node == nullptr) {
      return false;
    }

    return final_node->is_end_of_word;
  }

  bool startsWith(const std::string &prefix) const {
    return find_last_node(prefix) != nullptr;
  }

  bool remove(const std::string &word) {
    // TODO: Remove word from the trie. Return true if the word was present.
    return false;
  }

  std::vector<std::string> getWordsWithPrefix(const std::string &prefix) const {
    // TODO: Return all stored words that start with prefix.
    return {};
  }

private:
  const Node *find_last_node(const std::string &word) const {
    auto current_node_ptr = nodes.front().get();
    for (const auto character : word) {
      auto child_node_ptr = current_node_ptr->children[character - 'a'];
      if (child_node_ptr == nullptr) {
        return nullptr;
      }
      current_node_ptr = child_node_ptr;
    }
    return current_node_ptr;
  }

  std::vector<std::unique_ptr<Node>> nodes;
};

class PtrTrie {
  struct Node {
    bool is_end_of_word = false;
    std::array<std::unique_ptr<Node>, 26> children{};
  };

public:
  PtrTrie() : root(std::make_unique<Node>()) {}

  void insert(const std::string &word) {
    // TODO
  }

  bool search(const std::string &word) const {
    // TODO
    return false;
  }

  bool startsWith(const std::string &prefix) const {
    // TODO
    return false;
  }

  bool remove(const std::string &word) {
    // TODO: Remove word from the trie. Return true if the word was present.
    return false;
  }

  std::vector<std::string> getWordsWithPrefix(const std::string &prefix) const {
    // TODO: Return all stored words that start with prefix.
    return {};
  }

private:
  std::unique_ptr<Node> root;
};
