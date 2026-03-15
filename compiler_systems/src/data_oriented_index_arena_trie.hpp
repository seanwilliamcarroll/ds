#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class DataOrientedIndexArenaTrie {
  struct Node {
    static constexpr size_t VOCABULARY_LENGTH = 26;
    static constexpr char FIRST_LETTER = 'a';
    static constexpr size_t NULL_INDEX = 0ULL;
    // bool is_end_of_word = false;
    std::array<size_t, VOCABULARY_LENGTH> children;

    Node() {
      for (auto &child : children) {
        child = NULL_INDEX;
      }
    }

    void set_child_index(char character, size_t child_index) {
      children[static_cast<size_t>(character - FIRST_LETTER)] = child_index;
    }

    size_t get_child_index(char character) const {
      return children[static_cast<size_t>(character - FIRST_LETTER)];
    }

    size_t get_child_index_by_index(size_t index) const {
      return children[index];
    }
  };

  static constexpr size_t ROOT_NODE_INDEX = 1ULL;

public:
  // Opaque handle for incremental trie traversal. Allows walking the trie
  // one character at a time without exposing Node internals.
  class Cursor {
  public:
    Cursor advance(char c) const {
      if (!is_valid()) {
        return Cursor(parent_trie, Node::NULL_INDEX);
      }
      auto child_index = parent_trie.nodes[node_index].get_child_index(c);
      return Cursor(parent_trie, child_index);
    }

    bool is_word() const {
      if (!is_valid()) {
        return false;
      }
      return parent_trie.end_of_word[node_index];
    }

    bool is_valid() const { return node_index != Node::NULL_INDEX; }

  private:
    friend class DataOrientedIndexArenaTrie;
    explicit Cursor(const DataOrientedIndexArenaTrie &parent_trie,
                    size_t node_index)
        : parent_trie(parent_trie), node_index(node_index) {}
    const DataOrientedIndexArenaTrie &parent_trie;
    const size_t node_index;
  };

  // Get a cursor pointing at the root of the trie.
  Cursor cursor() const { return Cursor(*this, ROOT_NODE_INDEX); }

  DataOrientedIndexArenaTrie() {
    nodes.push_back(Node());
    end_of_word.push_back(false);
    // The true root node
    nodes.push_back(Node());
    end_of_word.push_back(false);
  };

  void insert(const std::string &word) {
    auto current_node_index = ROOT_NODE_INDEX;
    for (const auto character : word) {
      size_t child_node_index =
          nodes[current_node_index].get_child_index(character);
      if (child_node_index == Node::NULL_INDEX) {
        child_node_index = nodes.size();
        nodes[current_node_index].set_child_index(character, nodes.size());
        nodes.push_back(Node());
        end_of_word.push_back(false);
      }
      current_node_index = child_node_index;
    }
    end_of_word[current_node_index] = true;
  }

  bool search(const std::string &word) const {
    auto final_node_index = find_last_node_index(word);
    if (final_node_index == Node::NULL_INDEX) {
      return false;
    }

    return end_of_word[final_node_index];
  }

  bool startsWith(const std::string &prefix) const {
    return find_last_node_index(prefix) != Node::NULL_INDEX;
  }

  bool remove(const std::string &word) {
    auto final_node_index = find_last_node_index(word);
    if (final_node_index == Node::NULL_INDEX) {
      return false;
    }
    bool was_present = end_of_word[final_node_index];
    end_of_word[final_node_index] = false;
    return was_present;
  }

  std::vector<std::string> getWordsWithPrefix(const std::string &prefix) const {
    auto end_of_prefix_index = find_last_node_index(prefix);
    if (end_of_prefix_index == Node::NULL_INDEX) {
      return {};
    }
    std::vector<std::string> output;
    // DFS our way through
    using IntermediateResult = std::pair<std::string, size_t>;
    std::vector<IntermediateResult> next_to_try{{prefix, end_of_prefix_index}};

    while (!next_to_try.empty()) {
      auto [possible_word, node_index] = next_to_try.back();
      next_to_try.pop_back();
      if (end_of_word[node_index]) {
        output.push_back(possible_word);
      }
      for (size_t character_index = 0;
           character_index < Node::VOCABULARY_LENGTH; ++character_index) {
        auto child_index =
            nodes[node_index].get_child_index_by_index(character_index);
        if (child_index == Node::NULL_INDEX) {
          continue;
        }
        const char next_letter = Node::FIRST_LETTER + char(character_index);
        next_to_try.push_back({possible_word + next_letter, child_index});
      }
    }

    return output;
  }

private:
  size_t find_last_node_index(const std::string &word) const {
    auto current_node_index = ROOT_NODE_INDEX;
    for (const auto character : word) {
      auto child_node_index =
          nodes[current_node_index].get_child_index(character);
      if (child_node_index == Node::NULL_INDEX) {
        return Node::NULL_INDEX;
      }
      current_node_index = child_node_index;
    }
    return current_node_index;
  }

  std::vector<Node> nodes;
  std::vector<bool> end_of_word;
};
