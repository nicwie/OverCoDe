#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "Graph.h"

// Helper class to access the protected static method
class GraphTestHelper : public Graph {
public:
  void generateGraph() override { /* Not needed for this test */ }

  // Expose the protected static method
  static void
  callGenerateCombinations(unsigned long long n, unsigned long long r,
                           std::vector<std::vector<int>> &combinations) {
    generateCombinations(n, r, combinations);
  }
};

TEST(GraphTest, GenerateCombinations) {
  std::vector<std::vector<int>> combinations;
  // Test C(4, 2)
  GraphTestHelper::callGenerateCombinations(4, 2, combinations);

  // Should be 6 combinations: {0,1}, {0,2}, {0,3}, {1,2}, {1,3}, {2,3}
  EXPECT_EQ(combinations.size(), 6);

  // Use std::set for order-independent comparison
  std::set<std::set<int>> combinationsSet;
  for (auto &comb : combinations) {
    combinationsSet.insert(std::set<int>(comb.begin(), comb.end()));
  }

  std::set<std::set<int>> expected = {{0, 1}, {0, 2}, {0, 3},
                                      {1, 2}, {1, 3}, {2, 3}};
  EXPECT_EQ(combinationsSet, expected);

  // Test C(3, 3)
  combinations.clear();
  GraphTestHelper::callGenerateCombinations(3, 3, combinations);
  EXPECT_EQ(combinations.size(), 1);
  EXPECT_EQ(std::set<int>(combinations[0].begin(), combinations[0].end()),
            std::set<int>({0, 1, 2}));
}
