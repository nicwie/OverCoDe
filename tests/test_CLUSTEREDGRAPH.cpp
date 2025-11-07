#include <algorithm>
#include <chrono>
#include <gtest/gtest.h>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "ClusteredGraph.h"

TEST(ClusteredGraphTest, NodeAndClusterSetup) {
  // 3 clusters, 10 nodes per cluster (target), 2-way overlap of 2, 3-way
  // overlap of 1
  std::vector<unsigned long long> overlaps = {
      0, 2, 1}; // nExclusive is at index 0, will be calculated
  size_t n = 10;

  ClusteredGraph graph(n, overlaps);
  graph.generateGraph();

  const auto &adjList = graph.getAdjList(); // This is public in Graph

  // Parse clusters from file
  const std::string tempFilename = "temp_clustertest.txt";
  graph.appendTruthToFile(tempFilename);

  std::vector<std::vector<unsigned long long>> parsedClusters;
  std::ifstream clusterFile(tempFilename);
  std::string line;

  ASSERT_TRUE(clusterFile.is_open())
      << "Test failed to open temp cluster file: " << tempFilename;

  while (std::getline(clusterFile, line)) {
    if (line.find("Cluster") != std::string::npos) {
      // Start of a new cluster
      parsedClusters.push_back({});
    } else if (!parsedClusters.empty() && !line.empty()) {
      // This line contains nodes for the last cluster
      std::stringstream ss(line);
      unsigned long long node;
      while (ss >> node) {
        parsedClusters.back().push_back(node);
      }
    }
  }
  clusterFile.close();
  std::remove(tempFilename.c_str()); // Clean up the temp file
  // End parsing

  // Check cluster count
  EXPECT_EQ(parsedClusters.size(), 3);

  // Calculate expected node counts
  // C(n-1, i) -> C(3-1, 1-1) = C(2,0) = 1 (not used)
  // C(3-1, 2-1) = C(2,1) = 2
  // C(3-1, 3-1) = C(2,2) = 1 (this is wrong, in code it's C(n-1, i))

  // From code: nExclusive = n - (overlaps[i]) * C((overlaps.size()) - 1, i)
  // nExclusive = 10 - (overlaps[1] * C(2,1)) - (overlaps[2] * C(2,2))
  // nExclusive = 10 - (2 * 2) - (1 * 1) = 10 - 4 - 1 = 5
  unsigned long long expectedNExclusive = 5;

  // From code: uniqueNodes = C(overlaps.size(), i + 1) * overlaps[i]
  // uniqueNodes = (C(3,1) * nExclusive) + (C(3,2) * overlaps[1]) + (C(3,3) *
  // overlaps[2]) uniqueNodes = (3 * 5) + (3 * 2) + (1 * 1) = 15 + 6 + 1 = 22
  unsigned long long expectedUniqueNodes = 22;

  EXPECT_EQ(adjList.size(), expectedUniqueNodes);

  // Check node distribution
  std::map<unsigned long long, int> nodeCounts;
  for (const auto &cluster : parsedClusters) { // Use parsedClusters
    for (unsigned long long node : cluster) {
      nodeCounts[node]++;
    }
  }

  int exclusiveCount = 0;
  int overlap2Count = 0;
  int overlap3Count = 0;

  for (const auto &pair : nodeCounts) {
    if (pair.second == 1)
      exclusiveCount++;
    else if (pair.second == 2)
      overlap2Count++;
    else if (pair.second == 3)
      overlap3Count++;
  }

  // 3 clusters * 5 exclusive nodes/cluster = 15 nodes that appear once
  EXPECT_EQ(exclusiveCount, expectedNExclusive * 3);
  // C(3,2) = 3 pairs. 3 pairs * 2 nodes/overlap = 6 nodes that appear twice
  EXPECT_EQ(overlap2Count, 6);
  // C(3,3) = 1 triplet. 1 triplet * 1 node/overlap = 1 node that appears thrice
  EXPECT_EQ(overlap3Count, 1);
}
