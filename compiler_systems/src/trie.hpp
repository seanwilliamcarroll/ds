#pragma once

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class ArenaTrie {
  struct Node {
    static constexpr size_t VOCABULARY_LENGTH = 26;
    static constexpr char FIRST_LETTER = 'a';
    bool is_end_of_word = false;
    std::array<Node *, VOCABULARY_LENGTH> children{};
  };

public:
  ArenaTrie() { nodes.push_back(std::make_unique<Node>()); };

  void insert(const std::string &word) {
    auto current_node_ptr = nodes.front().get();
    for (const auto character : word) {
      auto &child_node_ptr =
          current_node_ptr->children[character - Node::FIRST_LETTER];
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
    // With the arena, is there a strategy to reap empty nodes?
    // Or could possibly grow and grow after many inserts and removals?
    // Would depend on intended use cases I suppose
    Node *final_node = find_last_node(word);
    if (final_node == nullptr) {
      return false;
    }
    auto was_present = final_node->is_end_of_word;
    final_node->is_end_of_word = false;
    return was_present;
  }

  std::vector<std::string> getWordsWithPrefix(const std::string &prefix) const {
    const Node *end_of_prefix = find_last_node(prefix);
    if (end_of_prefix == nullptr) {
      return {};
    }
    std::vector<std::string> output;
    // DFS our way through
    using IntermediateResult = std::pair<std::string, const Node *>;
    std::vector<IntermediateResult> next_to_try{{prefix, end_of_prefix}};

    while (!next_to_try.empty()) {
      auto [possible_word, node_ptr] = next_to_try.back();
      next_to_try.pop_back();
      if (node_ptr->is_end_of_word) {
        output.push_back(possible_word);
      }
      for (size_t child_index = 0; child_index < Node::VOCABULARY_LENGTH;
           ++child_index) {
        auto child = node_ptr->children[child_index];
        if (child == nullptr) {
          continue;
        }
        const char next_letter = Node::FIRST_LETTER + char(child_index);
        next_to_try.push_back({possible_word + next_letter, child});
      }
    }

    return output;
  }

private:
  const Node *find_last_node(const std::string &word) const {
    auto current_node_ptr = nodes.front().get();
    for (const auto character : word) {
      auto child_node_ptr =
          current_node_ptr->children[character - Node::FIRST_LETTER];
      if (child_node_ptr == nullptr) {
        return nullptr;
      }
      current_node_ptr = child_node_ptr;
    }
    return current_node_ptr;
  }

  Node *find_last_node(const std::string &word) {
    return const_cast<Node *>(
        static_cast<const ArenaTrie *>(this)->find_last_node(word));
  }

  std::vector<std::unique_ptr<Node>> nodes;
};

class PtrTrie {
  struct Node {
    static constexpr size_t VOCABULARY_LENGTH = 26;
    static constexpr char FIRST_LETTER = 'a';
    bool is_end_of_word = false;
    std::array<std::unique_ptr<Node>, VOCABULARY_LENGTH> children{};
  };

public:
  PtrTrie() : root(std::make_unique<Node>()) {}

  void insert(const std::string &word) {
    auto root_ptr = root.get();
    for (const auto character : word) {
      auto &child_ptr = root_ptr->children[character - Node::FIRST_LETTER];
      if (child_ptr == nullptr) {
        child_ptr = std::make_unique<Node>();
      }
      root_ptr = child_ptr.get();
    }
    root_ptr->is_end_of_word = true;
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
    // Want to walk down list and keep track of parent chain,
    // apple -> appl -> app -> ap -> a -> [root]
    // then walk it back
    // until either root node or we find a node with other children, removing
    // the ones without children and setting the root node back to no children
    auto current_node_unique_ptr = &root;
    std::vector<std::unique_ptr<Node> *> parent_chain;
    for (const auto character : word) {
      auto current_node_ptr = current_node_unique_ptr->get();
      auto child_unique_ptr =
          &current_node_ptr->children[character - Node::FIRST_LETTER];
      auto child_ptr = child_unique_ptr->get();
      if (child_ptr == nullptr) {
        return false;
      }
      parent_chain.push_back(child_unique_ptr);
      current_node_unique_ptr = child_unique_ptr;
    }
    // parent_chain should not contain root, but all nodes leading to root
    auto current_node_ptr = current_node_unique_ptr->get();
    auto was_present = current_node_ptr->is_end_of_word;
    current_node_ptr->is_end_of_word = false;
    while (!parent_chain.empty()) {
      // If top of stack has children or is end of word, break
      // else reset it
      auto next_node_unique_ptr = parent_chain.back();
      parent_chain.pop_back();
      auto next_node = next_node_unique_ptr->get();
      if (next_node->is_end_of_word) {
        return was_present;
      }
      for (const auto &child_unique_ptr : next_node->children) {
        if (child_unique_ptr != nullptr) {
          return was_present;
        }
      }
      next_node_unique_ptr->reset();
    }
    return was_present;
  }

  std::vector<std::string> getWordsWithPrefix(const std::string &prefix) const {
    const Node *end_of_prefix = find_last_node(prefix);
    if (end_of_prefix == nullptr) {
      return {};
    }
    std::vector<std::string> output;
    // DFS our way through
    using IntermediateResult = std::pair<std::string, const Node *>;
    std::vector<IntermediateResult> next_to_try{{prefix, end_of_prefix}};

    while (!next_to_try.empty()) {
      auto [possible_word, node_ptr] = next_to_try.back();
      next_to_try.pop_back();
      if (node_ptr->is_end_of_word) {
        output.push_back(possible_word);
      }
      for (size_t child_index = 0; child_index < Node::VOCABULARY_LENGTH;
           ++child_index) {
        auto child = node_ptr->children[child_index].get();
        if (child == nullptr) {
          continue;
        }
        const char next_letter = Node::FIRST_LETTER + char(child_index);
        next_to_try.push_back({possible_word + next_letter, child});
      }
    }

    return output;
  }

private:
  const Node *find_last_node(const std::string &word) const {
    auto current_node_ptr = root.get();
    for (const auto character : word) {
      auto &child_node_ptr =
          current_node_ptr->children[character - Node::FIRST_LETTER];
      if (child_node_ptr == nullptr) {
        return nullptr;
      }
      current_node_ptr = child_node_ptr.get();
    }
    return current_node_ptr;
  }

  Node *find_last_node(const std::string &word) {
    return const_cast<Node *>(
        static_cast<const PtrTrie *>(this)->find_last_node(word));
  }
  std::unique_ptr<Node> root;
};
