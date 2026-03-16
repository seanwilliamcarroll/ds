#pragma once

#include <array>
#include <deque>
#include <string>
#include <utility>
#include <vector>

class DequeArenaTrie {
  struct Node {
    static constexpr size_t VOCABULARY_LENGTH = 26;
    static constexpr char FIRST_LETTER = 'a';
    bool is_end_of_word = false;
    std::array<Node *, VOCABULARY_LENGTH> children{};

    [[nodiscard]] Node *const &get_child(char character) const {
      return children[static_cast<size_t>(character - FIRST_LETTER)];
    }

    Node *&get_child(char character) {
      return const_cast<Node *&>(
          static_cast<const Node *>(this)->get_child(character));
    }

    [[nodiscard]] Node *const &get_child_by_index(size_t index) const {
      return children[index];
    }

    Node *&get_child_by_index(size_t index) {
      return const_cast<Node *&>(
          static_cast<const Node *>(this)->get_child_by_index(index));
    }
  };

public:
  class Cursor {
  public:
    [[nodiscard]] Cursor advance(char c) const {
      if (!is_valid()) {
        return Cursor(nullptr);
      }
      auto *child = node_->get_child(c);
      return Cursor(child);
    }

    [[nodiscard]] bool is_word() const {
      if (!is_valid()) {
        return false;
      }
      return node_->is_end_of_word;
    }

    [[nodiscard]] bool is_valid() const { return node_ != nullptr; }

  private:
    friend class DequeArenaTrie;
    explicit Cursor(const Node *node) : node_(node) {}
    const Node *node_;
  };

  [[nodiscard]] Cursor cursor() const { return Cursor(&nodes.front()); }

  DequeArenaTrie() { nodes.push_back({}); };

  void insert(const std::string &word) {
    auto *current_node_ptr = &nodes.front();
    for (const auto character : word) {
      auto &child_node_ptr = current_node_ptr->get_child(character);
      if (child_node_ptr == nullptr) {
        nodes.push_back({});
        child_node_ptr = &nodes.back();
      }
      current_node_ptr = child_node_ptr;
    }
    current_node_ptr->is_end_of_word = true;
  }

  [[nodiscard]] bool search(const std::string &word) const {
    const Node *final_node = find_last_node(word);
    if (final_node == nullptr) {
      return false;
    }

    return final_node->is_end_of_word;
  }

  [[nodiscard]] bool startsWith(const std::string &prefix) const {
    return find_last_node(prefix) != nullptr;
  }

  bool remove(const std::string &word) {
    Node *final_node = find_last_node(word);
    if (final_node == nullptr) {
      return false;
    }
    auto was_present = final_node->is_end_of_word;
    final_node->is_end_of_word = false;
    return was_present;
  }

  [[nodiscard]] std::vector<std::string>
  getWordsWithPrefix(const std::string &prefix) const {
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
        auto *child = node_ptr->get_child_by_index(child_index);
        if (child == nullptr) {
          continue;
        }
        const char next_letter = static_cast<char>(
            Node::FIRST_LETTER + static_cast<char>(child_index));
        next_to_try.emplace_back(possible_word + next_letter, child);
      }
    }

    return output;
  }

private:
  [[nodiscard]] const Node *find_last_node(const std::string &word) const {
    const auto *current_node_ptr = &nodes.front();
    for (const auto character : word) {
      auto *child_node_ptr = current_node_ptr->get_child(character);
      if (child_node_ptr == nullptr) {
        return nullptr;
      }
      current_node_ptr = child_node_ptr;
    }
    return current_node_ptr;
  }

  Node *find_last_node(const std::string &word) {
    return const_cast<Node *>(
        static_cast<const DequeArenaTrie *>(this)->find_last_node(word));
  }

  std::deque<Node> nodes;
};
