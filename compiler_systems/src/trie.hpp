#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

class Trie {
  struct Node {
    bool is_end_of_word = false;
    std::array<Node *, 26> children{};
  };

public:
  Trie() { nodes.push_back(std::make_unique<Node>()); };

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
