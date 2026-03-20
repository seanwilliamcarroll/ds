#pragma once

#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

template <bool UseSentinelValue = true> class IndexArenaTrie {
  struct Node {
    static constexpr size_t VOCABULARY_LENGTH = 26;
    static constexpr char FIRST_LETTER = 'a';
    static constexpr size_t NULL_INDEX =
        UseSentinelValue ? std::numeric_limits<size_t>::max() : 0ULL;
    bool is_end_of_word = false;
    std::array<size_t, VOCABULARY_LENGTH> children{};

    Node() {
      for (auto &child : children) {
        child = NULL_INDEX;
      }
    }

    void set_child_index(char character, size_t child_index) {
      children[static_cast<size_t>(character - FIRST_LETTER)] = child_index;
    }

    [[nodiscard]] size_t get_child_index(char character) const {
      return children[static_cast<size_t>(character - FIRST_LETTER)];
    }

    [[nodiscard]] size_t get_child_index_by_index(size_t index) const {
      return children[index];
    }
  };

  static constexpr size_t ROOT_NODE_INDEX = UseSentinelValue ? 0ULL : 1ULL;

public:
  // Opaque handle for incremental trie traversal. Allows walking the trie
  // one character at a time without exposing Node internals.
  class Cursor {
  public:
    // Advance to the child for the given character.
    // Returns a new Cursor, or an invalid Cursor if no such child exists.
    Cursor advance(char c) const {
      if (!is_valid()) {
        return Cursor(parent_trie, Node::NULL_INDEX);
      }
      auto child_index = parent_trie.nodes[node_index].get_child_index(c);
      return Cursor(parent_trie, child_index);
    }

    // Is this cursor pointing at a complete word?
    [[nodiscard]] bool is_word() const {
      if (!is_valid()) {
        return false;
      }
      return parent_trie.nodes[node_index].is_end_of_word;
    }

    // Is this cursor valid (i.e. pointing at a real node)?
    [[nodiscard]] bool is_valid() const {
      return node_index != Node::NULL_INDEX;
    }

  private:
    friend class IndexArenaTrie;

    explicit Cursor(const IndexArenaTrie<UseSentinelValue> &parent_trie,
                    size_t node_index)
        : parent_trie(parent_trie), node_index(node_index) {}

    const IndexArenaTrie<UseSentinelValue> &parent_trie;
    const size_t node_index;
  };

  // Get a cursor pointing at the root of the trie.
  Cursor cursor() const { return Cursor(*this, ROOT_NODE_INDEX); }

  IndexArenaTrie() {
    nodes.push_back(Node());
    if constexpr (!UseSentinelValue) {
      // The true root node
      nodes.push_back(Node());
    }
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
      }
      current_node_index = child_node_index;
    }
    nodes[current_node_index].is_end_of_word = true;
  }

  [[nodiscard]] bool search(const std::string &word) const {
    auto final_node_index = find_last_node_index(word);
    if (final_node_index == Node::NULL_INDEX) {
      return false;
    }

    return nodes[final_node_index].is_end_of_word;
  }

  [[nodiscard]] bool starts_with(const std::string &prefix) const {
    return find_last_node_index(prefix) != Node::NULL_INDEX;
  }

  bool remove(const std::string &word) {
    auto final_node_index = find_last_node_index(word);
    if (final_node_index == Node::NULL_INDEX) {
      return false;
    }
    auto was_present = nodes[final_node_index].is_end_of_word;
    nodes[final_node_index].is_end_of_word = false;
    return was_present;
  }

  [[nodiscard]] std::vector<std::string>
  get_words_with_prefix(const std::string &prefix) const {
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
      if (nodes[node_index].is_end_of_word) {
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
  [[nodiscard]] size_t find_last_node_index(const std::string &word) const {
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
};
