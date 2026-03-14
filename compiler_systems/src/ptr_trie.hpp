#pragma once

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class PtrTrie {
  struct Node {
    static constexpr size_t VOCABULARY_LENGTH = 26;
    static constexpr char FIRST_LETTER = 'a';
    bool is_end_of_word = false;
    std::array<std::unique_ptr<Node>, VOCABULARY_LENGTH> children{};

    const std::unique_ptr<Node> &get_child(char character) const {
      return children[static_cast<size_t>(character - FIRST_LETTER)];
    }

    std::unique_ptr<Node> &get_child(char character) {
      return const_cast<std::unique_ptr<Node> &>(
          static_cast<const Node *>(this)->get_child(character));
    }

    const std::unique_ptr<Node> &get_child_by_index(size_t index) const {
      return children[index];
    }

    std::unique_ptr<Node> &get_child_by_index(size_t index) {
      return const_cast<std::unique_ptr<Node> &>(
          static_cast<const Node *>(this)->get_child_by_index(index));
    }
  };

public:
  class Cursor {
  public:
    Cursor advance(char c) const;
    bool is_word() const;
    bool is_valid() const;

  private:
    friend class PtrTrie;
    explicit Cursor(const Node *node);
    const Node *node_;
  };

  Cursor cursor() const;

  PtrTrie() : root(std::make_unique<Node>()) {}

  void insert(const std::string &word) {
    auto current_node_ptr = root.get();
    for (const auto character : word) {
      auto &child_ptr = current_node_ptr->get_child(character);
      if (child_ptr == nullptr) {
        child_ptr = std::make_unique<Node>();
      }
      current_node_ptr = child_ptr.get();
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
    auto current_node_unique_ptr = &root;
    std::vector<std::unique_ptr<Node> *> parent_chain;
    for (const auto character : word) {
      auto current_node_ptr = current_node_unique_ptr->get();
      auto child_unique_ptr = &current_node_ptr->get_child(character);
      auto child_ptr = child_unique_ptr->get();
      if (child_ptr == nullptr) {
        return false;
      }
      parent_chain.push_back(child_unique_ptr);
      current_node_unique_ptr = child_unique_ptr;
    }
    auto current_node_ptr = current_node_unique_ptr->get();
    auto was_present = current_node_ptr->is_end_of_word;
    current_node_ptr->is_end_of_word = false;
    while (!parent_chain.empty()) {
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
        auto child = node_ptr->get_child_by_index(child_index).get();
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
      auto &child_node_ptr = current_node_ptr->get_child(character);
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
