#include <cstdlib>
#include <iostream>
#include <vector>
#include <cassert>
#include <stack>
#include <utility>

template <typename T_Key, typename T_Value>
class RadixTree {
public:
  bool exists(const T_Key& key) const {
    const auto node = find_node(key);
    return node != nullptr;
  };

  /**
   * Returns T_Value() if the key was not found.
   */
  T_Value get_value(const T_Key& key) const {
    const auto node = find_node(key);
    return node ? node->value : T_Value();
  }

  std::vector<T_Key> find_candidates(const T_Key& prefix) {
    if (prefix.empty()) {
      return {};
    }

    const auto prefix_node = find_node(prefix, false /* not just leaves */);
    if (!prefix_node) {
      return {};
    }

    std::vector<T_Key> result;

    //Stack of prefixes+nodes.
    std::stack<std::pair<T_Key, const Node*>> stack;
    stack.emplace(prefix, prefix_node);

    while (!stack.empty()) {
      const auto item = stack.top();
      stack.pop();

      const auto& node = item.second;
      if (node->children.empty()) {
        result.emplace_back(item.first);
      }

      for (auto edge : node->children) {
        stack.emplace(item.first + edge.part, edge.dest);
      }
    }

    return result;
  }

  void insert(const T_Key& key, const T_Value& value) {
    // std::cout << "insert(): key=" << key << std::endl;
    if (key.empty()) {
      return;
    }

    auto node = &root;
    std::size_t key_pos = 0;
    const auto key_size = key.size();
    while (key_pos < key_size) {
      //std::cout << "insert(): remaining key=" << key.substr(key_pos) << std::endl;
      //Choose the child node, if any:
      Node* next = nullptr;
      for (auto& edge : node->children) {
        const auto& part = edge.part;
        //std::cout << "key=" << key << ", key_pos=" << key_pos << ", part=" << part << "\n";
        //If the edge's part is a prefix of the remaining key:
        if (has_prefix(key, key_pos, part)) {
          //std::cout << "  has_prefix.\n";
          next = edge.dest;
          key_pos += part.size();
          break;
        } else {
          // If the key is a prefix of the edge's part:
          std::size_t prefix_len = 0; // TODO: With C++17, put this in the if()
          if ((prefix_len = common_prefix(part, 0, key, key_pos))) {
            // Split it,
            // adding a new intermediate node in it original node's place, with the original node as a child.
            const auto prefix = key.substr(0, prefix_len);
            //std::cout << "  splitting with prefix=" << prefix << std::endl;
            const auto suffix_part = part.substr(prefix_len);
            //std::cout << "    suffix_part=" << suffix_part << std::endl;

            const auto dest = edge.dest;

            auto extra_node = new Node;
            typename Node::Edge extra_edge;
            extra_edge.part = suffix_part;
            extra_edge.dest = dest;
            extra_node->children.emplace_back(extra_edge);

            edge.part = prefix;
            edge.dest = extra_node;

            next = edge.dest;
            key_pos += prefix_len;
            break;
          }
        }
      }

      // Stop when we cannot go further.
      if (!next) {
        break;
      }

      node = next;
    }

    if (key_pos > key_size) {
      std::cerr << "Unexpected key_pos.\n";
      return;
    }

    if (key_pos == key_size) {
      // TODO: Return false because it is already there.
      //std::cout << "debug: insert(): Already exists." << std::endl;
      return;
    }

    // Add a node for the remaining characters:
    const auto suffix = key.substr(key_pos);
    //std::cout << "Adding suffix: " << suffix << ", with value: " << value << '\n';

    const auto next = new Node;
    typename Node::Edge edge;
    edge.part = suffix;
    edge.dest = next;
    node->children.emplace_back(edge);

    next->is_leaf = true;
    next->value = value;
    //std::cout << "next: " << next << std::endl;
  }

private:
  class Node {
  public:
    class Edge {
    public:
      T_Key part = T_Key();
      Node* dest = nullptr;
    };

    //We could instead have a std::vector<Node*> children,
    //of size alphabet (such as 26),
    //to allow O(1) lookup, at the cost of wasted space.
    std::vector<Edge> children;

    // TODO: Wastes space on non-leaves.
    bool is_leaf = false;
    T_Value value = 0;
  };

  static
  bool has_prefix(const std::string& str, std::size_t str_start_pos, const std::string& prefix) {
    return str.compare(str_start_pos, prefix.size(),
        prefix, 0, std::string::npos /* TODO: Shoul not be necessary with C++14 */) == 0;
  }

public:
  /** Returns the number of characters at the end of the prefix that do not match the @a str from position
   * @a str_start_pos.
   *
   * @param match Whether @a str has the @a prefix.
   */
  static
  bool prefix_matches(const std::string& str, std::size_t str_start_pos, const std::string& prefix, std::size_t prefix_start_pos) {
    const auto str_len = str.size() - str_start_pos;
    const auto prefix_len = prefix.size() - prefix_start_pos;

    // prefix cannot be a prefix of str if it is longer than str:
    if (prefix_len > str_len) {
      return false;
    }

    const auto len = std::min(str_len, prefix_len);
    const auto end = str_start_pos + len;

    auto prefix_iter = std::begin(prefix) + prefix_start_pos;
    std::size_t i = str_start_pos;
    for (; i < end; ++i) {
      if (str[i] != *prefix_iter) {
        // Some part of the prefix doesn't match:
        return false;
      }

      ++prefix_iter;
    }

    //All characters in prefix match at the start of str:
    return true;
  }

  static
  std::size_t common_prefix(const std::string& str, std::size_t str_start_pos, const std::string& prefix, std::size_t prefix_start_pos) {
    //TODO: Use std::mismatch().

    const auto str_len = str.size() - str_start_pos;
    const auto prefix_len = prefix.size() - prefix_start_pos;

    const auto len = std::min(str_len, prefix_len);
    const auto str_end = str_start_pos + len;

    std::size_t i_str = str_start_pos;
    std::size_t i_prefix = prefix_start_pos;
    while(i_str < str_end) {
      if(str[i_str] != prefix[i_prefix]) {
        break;
      }

      ++i_str;
      ++i_prefix;
    }

    return i_str - str_start_pos;
  }

  const Node* find_node(const T_Key& key, bool leaf_only = true) const {
    //std::cout << "find_node(): key=" << key << std::endl;
    if (key.empty()) {
      return nullptr;
    }

    auto node = &root;
    std::size_t key_pos = 0;
    const auto key_size = key.size();
    while (key_pos < key_size) {
      //std::cout << "find_node(): remaining key=" << key.substr(key_pos) << std::endl;
      //std::cout << "  children size: " << node->children.size() << std::endl;
      //Choose the child node, if any:
      Node* next = nullptr;
      for (const auto& edge : node->children) {
        const auto& part = edge.part;
        const auto part_size = part.size();
        //std::cout << "key=" << key << ", key_pos=" << key_pos << ", part=" << part << "\n";
        if(!prefix_matches(key, key_pos, part, 0)) {
          continue;
        }

        next = edge.dest;
        key_pos += part_size;
        //std::cout << "    next: " << next << std::endl;
        break;
      }

      if (!next) {
        return nullptr;
      }

      node = next;
    }

    if (key_pos < key_size) {
      //We didn't find all the parts of the prefix:
      return nullptr;
    }

    //std::cout << "node: " << node << std::endl;
    return (!leaf_only || node->is_leaf) ? node : nullptr;
  }

  Node root;
};

void test_prefix_matches() {
  using Tree = RadixTree<std::string, int>;

  assert(Tree::prefix_matches("banana", 0, "banana", 0));
  assert(!Tree::prefix_matches("banan", 0, "banana", 0));
  assert(Tree::prefix_matches("banana", 0, "banan", 0));
  assert(Tree::prefix_matches("banana", 0, "ban", 0));
  assert(!Tree::prefix_matches("foo", 0, "banana", 0));
}

void test_common_prefix() {
  using Tree = RadixTree<std::string, int>;
  assert(Tree::common_prefix("banana", 0, "bandana", 0) == 3);
  assert(Tree::common_prefix("banana", 0, "foo", 0) == 0);
  assert(Tree::common_prefix("banana", 0, "banana", 0) == 6);
}

int main() {
  test_prefix_matches();
  test_common_prefix();

  RadixTree<std::string, int> radix_tree;
  radix_tree.insert("banana", 1);
  radix_tree.insert("bandana", 2);
  radix_tree.insert("foo", 3);
  radix_tree.insert("foobar", 4);

  assert(radix_tree.exists("foo"));
  assert(radix_tree.exists("banana"));
  assert(radix_tree.get_value("banana") == 1);
  assert(radix_tree.exists("bandana"));
  assert(radix_tree.get_value("bandana") == 2);
  assert(radix_tree.get_value("foo") == 3);

  assert(!radix_tree.exists("foop"));
  assert(radix_tree.get_value("foop") == 0);
  assert(!radix_tree.exists("ban"));
  assert(radix_tree.get_value("ban") == 0);

  const auto candidates = radix_tree.find_candidates("ban");
  // TODO: Check wthout caring about the order:
  const auto expected_candidates = std::vector<std::string>({"bandana", "banana"});
  /*
   for (const auto& candidate : candidates) {
    std::cout << "candidate: " << candidate << '\n';
  }
  */

  assert(candidates == expected_candidates);

  return EXIT_SUCCESS;
}

