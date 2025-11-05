#ifndef SYNTHETICEGOGRAPH_H
#define SYNTHETICEGOGRAPH_H

#include <vector>

#include "Graph.h"
#include "RandomGenerator.h"

class SyntheticEgoGraph final : public Graph {
private:
  // stores which nodes are in which cluster
  RandomGenerator rng;

  void addNodes() {
    // counter for where we are (what node we are considering)
    size_t usedNodes = 0;

    // add pure (non-overlapping) nodes to clusters
    for (auto &cluster : clusters) {
      const size_t clusterSize = static_cast<size_t>(rng.getNormalInt(125, 25));
      for (size_t j = usedNodes; j < (usedNodes + clusterSize); j++) {
        cluster.push_back(j);
      }
      usedNodes += clusterSize;
    }

    // add overlapping nodes to clusters

    // all possible combinations of r clusters
    for (size_t i = 0; i < clusters.size(); i++) {
      for (size_t j = i + 1; j < clusters.size(); j++) {
        const size_t overlapSize = static_cast<size_t>(rng.getRandomInt(0, 10));
        for (size_t m = usedNodes; m < (usedNodes + overlapSize); m++) {
          clusters[i].push_back(m);
          clusters[j].push_back(m);
        }
        usedNodes += overlapSize;
      }
    }

    // add ego Node once to a ll clusters
    for (std::vector<unsigned long long> &cluster : clusters) {
      cluster.push_back(usedNodes);
    }
  }

public:
  SyntheticEgoGraph() = default;

  void generateGraph() override {
    clusters.resize(static_cast<size_t>(rng.getRandomInt(4, 6)));
    addNodes();

    size_t graphSize = 0;

    for (const auto &cluster : clusters) {
      graphSize += cluster.size();
    }

    adjList.resize(graphSize);

    std::vector<std::vector<bool>>
        connected; // this stores nodes we have connected already, so that we do
                   // not connect nodes in the overlap > 1 time
    connected.resize(graphSize, std::vector<bool>(graphSize));

    // completely connect clusters internally

    for (auto &cluster : clusters) {
      for (size_t j = 0; j < cluster.size(); j++) {
        for (size_t m = j + 1; m < cluster.size(); m++) {
          if (!connected[cluster[m]][cluster[j]]) {
            connected[cluster[j]][cluster[m]] = true;
            connected[cluster[m]][cluster[j]] = true;
            adjList[cluster[j]].push_back(cluster[m]);
            adjList[cluster[m]].push_back(cluster[j]);
          }
        }
      }
    }

    // add 10-20 edges between any 2 pairs of clusters
    for (size_t i = 0; i < clusters.size(); i++) {
      for (size_t j = i + 1; j < clusters.size(); j++) {
        const int edgeCount = rng.getRandomInt(10, 20);
        for (int m = 0; m < edgeCount; m++) {
          unsigned long long addToI =
              clusters[i][rng.getRandomUll(0, clusters[i].size() - 1)];
          unsigned long long addToJ =
              clusters[j][rng.getRandomUll(0, clusters[j].size() - 1)];

          if (!connected[addToI][addToJ]) {
            connected[addToI][addToJ] = true;
            connected[addToJ][addToI] = true;
            adjList[addToI].push_back(addToJ);
            adjList[addToJ].push_back(addToI);
          }
        }
      }
    }
  }

  std::vector<std::vector<unsigned long long>> getClusters() const {
    return clusters;
  }
};

#endif // SYNTHETICEGOGRAPH_H
