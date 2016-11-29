#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cassert>

#include <murrayc-suffix-tree/suffix_tree.h>

#include <boost/timer/timer.hpp>

static
void test_simple_single() {
  using Tree = SuffixTree<std::string, std::size_t>;
  Tree suffix_tree;

  suffix_tree.insert("xyzxyaxyz", 0);

  {
    auto results = suffix_tree.find("bob");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 0);
  }

  {
    auto results = suffix_tree.find("an");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 0);
  }

  {
    auto results = suffix_tree.find("zx");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 1);
    assert(results == Tree::Candidates({0}));
    for (const auto& result : results) {
      std::cout << result << ": " << std::endl;
    }
  }
}

static
void test_simple_multiple() {
  using Tree = SuffixTree<std::string, std::size_t>;
  Tree suffix_tree;

  suffix_tree.insert("banana", 0);
  suffix_tree.insert("bandana", 1);
  suffix_tree.insert("bar", 2);
  suffix_tree.insert("foobar", 3);

  auto results = suffix_tree.find("an");
  std::cout << "results.size(): " << results.size() << std::endl;
  assert(results.size() == 2);
  assert(results == Tree::Candidates({0, 1}));
  for (const auto& result : results) {
    std::cout << result << ": " << std::endl;
  }

  results = suffix_tree.find("bar");
  std::cout << "results.size(): " << results.size() << std::endl;
  assert(results.size() == 2);
  assert(results == Tree::Candidates({2, 3}));
}

static
void test_full_text_index_individual_strings() {
  std::ifstream in;
  in.open("tests/test_pg1400.txt");
  assert(in.is_open());

  // The actual strings are stored outside of the SuffixTree,
  // and must exist for as long as the SuffixTree is used.
  std::vector<std::string> strings;
  while (in) {
    std::string str;
    in >> str;
    strings.emplace_back(str);
  }
  in.close();

  std::cout << "SuffixTree: Construction:" << std::endl;
  boost::timer::auto_cpu_timer timer;
  using Tree = SuffixTree<std::string, std::size_t>;
  Tree suffix_tree;
  std::size_t pos = 0;
  for (const auto& str : strings) {
    suffix_tree.insert(str, pos);
    ++pos;
  }
  timer.stop();
  timer.report();

  std::cout << "SuffixTree: Search:" << std::endl;
  timer.start();
  const auto results = suffix_tree.find("xio");
  timer.stop();
  timer.report();

  assert(results.size() > 10); //TODO: Exact.
  for (const auto& result : results) {
    std::cout << result << ": " << strings[result] << std::endl;
  }
}

static
void test_full_text_index_one_string() {
  // Load the whole text file into one std::string.
  std::string str;
  std::ifstream in;
  in.open("tests/test_pg1400.txt");
  assert(in.is_open());
  in.seekg(0, std::ios::end);
  str.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&str[0], str.size());
  in.close();

  std::cout << "SuffixTree: Construction:" << std::endl;
  boost::timer::auto_cpu_timer timer;
  using Tree = SuffixTree<std::string, std::size_t>;
  Tree suffix_tree;

  // The actual strings are stored outside of the SuffixTree,
  // and must exist for as long as the SuffixTree is used.

  // Parse the text to find the words,
  // and add them to the SuffixTree.
  // TODO: Make SuffixTree: Support a T_Key type of const char*.
  const auto start = std::cbegin(str);
  const auto end = std::cend(str);
  auto pos = start;
  auto word_start = pos;
  std::size_t i = 0;
  while (pos < end) {
    if (std::isspace(*pos)) {
      const auto word_end = pos;
      suffix_tree.insert(word_start, word_end, i);
      word_start = word_end + 1;
      ++i;
    }

    ++pos;
  }

  timer.stop();
  timer.report();

  std::cout << "SuffixTree: Search:" << std::endl;
  timer.start();
  const auto results = suffix_tree.find("xio");
  timer.stop();
  timer.report();

  assert(results.size() > 10); //TODO: Exact.
  for (const auto& result : results) {
    std::cout << result << std::endl;
  }
}

static
void test_simple_single_with_position() {
  using Tree = SuffixTree<std::string, std::size_t>;
  Tree suffix_tree;

  const std::string str = "xyzxyaxyz";
  suffix_tree.insert(str, 0);

  {
    auto results = suffix_tree.find_with_position("bob");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 0);
  }

  {
    auto results = suffix_tree.find_with_position("an");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 0);
  }

  {
    auto results = suffix_tree.find_with_position("zx");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 1);

    const Tree::Range expected_range(std::cbegin(str) + 2, std::cend(str));
    const Tree::CandidatesAndPosition expected = {{expected_range, 0}};
    assert(results == expected);
    for (const auto& result : results) {
      const auto& range = result.first;
      const auto& value = result.second;
      std::cout << std::distance(std::cbegin(str), range.start_) << ": "
        << std::string(range.start_, range.end_) << ": " << value << std::endl;
    }
  }
}

static
void test_simple_multiple_with_position() {
  using Tree = SuffixTree<std::string, std::size_t>;
  Tree suffix_tree;

  // We keep the strings alive,
  // and just pass a reference,
  // so we can use the iterators that will
  // be returned by find_with_position()
  const std::string str1 = "banana";
  suffix_tree.insert(str1, 0);
  const std::string str2 = "bandana";
  suffix_tree.insert(str2, 1);
  const std::string str3 = "bar";
  suffix_tree.insert(str3, 2);
  const std::string str4 = "foobar";
  suffix_tree.insert(str4, 3);

  {
    const auto results = suffix_tree.find_with_position("an");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 4);
    // TODO: Don't test the order:
    const Tree::CandidatesAndPosition expected = {
      {Tree::Range(std::cbegin(str2) + 1, std::cend(str2)), 1},
      {Tree::Range(std::cbegin(str1) + 3, std::cend(str1)), 0},
      {Tree::Range(std::cbegin(str2) + 4, std::cend(str2)), 1},
      {Tree::Range(std::cbegin(str1) + 1, std::cend(str1)), 0}
    };
    assert(results == expected);

    for (const auto& result : results) {
      const auto& range = result.first;
      const auto& value = result.second;
      //std::cout << std::distance(std::cbegin(str), range.start_) << ": "
      std::cout << std::string(range.start_, range.end_) << ": " << value << std::endl;
    }
  }

  {
    const auto results = suffix_tree.find_with_position("bar");
    std::cout << "results.size(): " << results.size() << std::endl;
    assert(results.size() == 2);
    const Tree::CandidatesAndPosition expected = {
      {Tree::Range(std::cbegin(str3) + 0, std::cend(str3)), 2},
      {Tree::Range(std::cbegin(str4) + 3, std::cend(str4)), 3}
    };
    assert(results == expected);

    for (const auto& result : results) {
      const auto& range = result.first;
      const auto& value = result.second;
      //std::cout << std::distance(std::cbegin(str), range.start_) << ": "
      std::cout << std::string(range.start_, range.end_) << ": " << value << std::endl;
    }
  }
}

int main() {
  test_simple_single();
  test_simple_multiple();

  test_full_text_index_individual_strings();
  test_full_text_index_one_string();

  test_simple_single_with_position();
  test_simple_multiple_with_position();

  return EXIT_SUCCESS;
}

