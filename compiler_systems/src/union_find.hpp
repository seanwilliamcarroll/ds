#pragma once

#include <unordered_set>

// Union-Find (Disjoint Set Union)
//
// TODO: Implement with path compression + union by rank.

class UnionFind {
  struct Component {
    int representative;
    size_t size;
    std::unordered_set<int> elements;

    Component(int initial_node)
        : representative(initial_node), size(0), elements{initial_node} {}
  };

public:
  explicit UnionFind(int n) {
    components.reserve(n);
    node_to_sets.reserve(n);
    for (int node = 0; node < n; ++node) {
      Component component(node);
      components.push_back(std::move(component));
      node_to_sets[node] = &(components.back());
    }
  }

  int find(int x) { return node_to_sets[x]->representative; }

  void unite(int x, int y) {
    if (connected(x, y)) {
      return;
    }

    // Find the sets, pop from one into the other, update nodes to point to this
    // set as they are transfered
    auto x_component = node_to_sets[x];
    auto y_component = node_to_sets[y];

    if (x_component->representative > y_component->representative) {
      std::swap(x_component, y_component);
      std::swap(x, y);
    }
    // x is less than y, align on x
    for (auto element : y_component->elements) {
      x_component->elements.insert(element);
      node_to_sets[element] = x_component;
    }
    // Could remove entry from components, but more trouble than it's worth?
  }

  bool connected(int x, int y) { return find(x) == find(y); }

  std::vector<Component> components;

  std::unordered_map<int, Component *> node_to_sets;
};
