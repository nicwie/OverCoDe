#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "SyntheticEgoGraph.h"

TEST(SyntheticEgoGraphTest, GraphProperties) {
  SyntheticEgoGraph graph;
  graph.generateGraph();

  const auto &adjList = graph.getAdjList();
  const auto &clusters = graph.getClusters();

  ASSERT_FALSE(adjList.empty());
  ASSERT_FALSE(clusters.empty());

  // Check for ego node (last node in 'clusters' lists)
  unsigned long long egoNode = clusters[0].back();
  EXPECT_GT(adjList.size(), egoNode); // Ego node should exist in adjList

  for (const auto &cluster : clusters) {
    ASSERT_FALSE(cluster.empty());
    // Check if the last node (ego node) is the same in all clusters
    EXPECT_EQ(cluster.back(), egoNode);
  }

  // Check for clique property (all nodes in a cluster are connected)
  for (const auto &cluster : clusters) {
    for (size_t i = 0; i < cluster.size(); ++i) {
      for (size_t j = i + 1; j < cluster.size(); ++j) {
        unsigned long long u = cluster[i];
        unsigned long long v = cluster[j];

        // Find v in u's adjacency list
        const auto &neighbors = adjList[u];
        bool found =
            std::find(neighbors.begin(), neighbors.end(), v) != neighbors.end();

        std::string errorMsg = "Node " + std::to_string(u) +
                               " not connected to " + std::to_string(v) +
                               " in cluster.";
        EXPECT_TRUE(found) << errorMsg;
      }
    }
  }
}
