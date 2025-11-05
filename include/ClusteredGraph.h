#ifndef CLUSTEREDGRAPH_H_INCLUDED
#define CLUSTEREDGRAPH_H_INCLUDED

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

#include "Graph.h"
#include "RandomGenerator.h"

inline bool contains1(const std::vector<unsigned long long> &g,
                      const unsigned long long n) {
  return std::any_of(g.begin(), g.end(), [&n](const unsigned long long x) {
    return x == static_cast<unsigned long long>(n);
  });
}

class ClusteredGraph final : public Graph {
private:
  size_t n; // Number of nodes in each cluster
  size_t clusterNr;
  double p;
  double q;
  double intraProb; // Probability of intra-cluster edges
  double interProb; // Probability of inter-cluster edges

  std::vector<unsigned long long>
      overlaps; // stores how large the overlap between clusters should be
                // (there is an overlap of size overlaps[n-1] between n
                // clusters)
  RandomGenerator rng;

  bool coinFlip(const double probability) {
    return (rng.getRandomDouble(0.0, 100.0) / 100.0) < probability;
  }

  // n Choose r
  static unsigned long long C(const unsigned long long n,
                              unsigned long long r) {
    if (r > n - r) {
      r = n - r; // because C(n, r) == C(n, n - r)
    }
    unsigned long long ans = 1;

    for (unsigned long long i = 1; i <= r; i++) {
      ans *= n - r + i;
      ans /= i;
    }

    return ans;
  }

  // adds node to clusters according to overlaps
  /* thinking for 3 clusters
      first we need to add overlap[0] (unique nodes) to each cluster
      then we need to add overlap[1] to all clusters as pairs 3nCr2 times
      then we need to add overlap[2] to all clusters at once
      so
      1 1 1
      2 2 2
      3
      nCr times

      for 4 we need
      1 1 1 1
      2 2 2 2 2 2
      3 3 3 3
      4
  */
  static void addNodes(std::vector<std::vector<unsigned long long>> &clusters,
                       const std::vector<unsigned long long> &overlaps) {
    unsigned long long usedNodes = 0;

    for (size_t i = 0; i < overlaps.size(); i++) {
      if (i == 0) {
        // add pure (non-overlapping) nodes to clusters
        for (size_t j = 0; j < clusters.size(); j++) {
          for (unsigned long long m = overlaps[i] * j;
               m < overlaps[i] * (j + 1); m++) {
            clusters[j].push_back(m);
          }
        }
        usedNodes += clusters.size() * overlaps[0];
      } else {
        // add overlapping nodes to clusters
        const unsigned long long r = i + 1;
        std::vector<std::vector<int>> combinations;

        // all possible combinations of r clusters
        generateCombinations(clusters.size(), r, combinations);

        for (unsigned long long m = 0; m < overlaps[i]; m++) {
          for (const auto &comb : combinations) {
            for (const int clusterId : comb) {
              clusters[static_cast<size_t>(clusterId)].push_back(usedNodes);
            }
            usedNodes++;
          }
        }
      }
    }
  }

public:
  ClusteredGraph(const size_t numNodes,
                 const std::vector<unsigned long long> &overlap)
      : Graph(), n(numNodes), clusterNr(overlap.size()),
        p(2.5 * pow(log2(static_cast<double>(n)) / static_cast<double>(n),
                    1 / static_cast<double>(4))),
        q(p / 150), intraProb(p), interProb(q), overlaps(overlap) {
    if (intraProb > 1) {
      intraProb = 1;
      interProb = static_cast<float>(1) / 150;
    }
    printProbabilities();
  }

  void generateGraph() override {

    unsigned long long nExclusive = n;

    // calculate how many nodes not in overlaps exist per cluster
    // example for 3 clusters:
    // int nExclusive = n - (overlap3 + (C(3 - 1, 2 - 1) * overlap2));
    for (size_t i = 1; i < overlaps.size(); i++) {
      nExclusive -= (overlaps[i]) * C((overlaps.size()) - 1, i);
    }

    overlaps[0] = nExclusive;

    unsigned long long uniqueNodes = 0;

    // calculate the total number of nodes
    // example for 3 clusters
    // int uniqueNodes = overlap3 + (C(3, 2) * overlap2) + (3 * nExclusive);
    for (size_t i = 0; i < overlaps.size(); i++) {
      uniqueNodes += C(overlaps.size(), i + 1) * overlaps[i];
    }

    adjList.resize(uniqueNodes);

    clusters.resize(clusterNr);

    std::vector<std::vector<bool>>
        connected; // this stores nodes we have connected already, so that we do
                   // not connect nodes in the overlap > 1 time
    connected.resize(uniqueNodes, std::vector<bool>(uniqueNodes));

    addNodes(clusters, overlaps);

    // now connecting clusters internally

    for (auto &cluster : clusters) {
      for (size_t j = 0; j < cluster.size(); j++) {
        for (size_t m = j + 1; m < cluster.size(); m++) {
          if (!connected[cluster[m]][cluster[j]]) {
            connected[cluster[j]][cluster[m]] = true;
            connected[cluster[m]][cluster[j]] = true;
            if (coinFlip(intraProb)) {
              adjList[cluster[j]].push_back(cluster[m]);
              adjList[cluster[m]].push_back(cluster[j]);
            }
          }
        }
      }
    }

    // connecting clusters externally as pairs

    for (size_t i = 0; i < clusters.size(); i++) {
      for (size_t j = i + 1; j < clusters.size(); j++) {
        for (size_t x = 0; x < clusters[i].size(); x++) {
          for (size_t y = 0; y < clusters[j].size(); y++) {
            if (!connected[clusters[j][y]][clusters[i][x]]) {
              connected[clusters[i][x]][clusters[j][y]] = true;
              connected[clusters[j][y]][clusters[i][x]] = true;
              if (coinFlip(interProb)) {
                adjList[clusters[i][x]].push_back(clusters[j][y]);
                adjList[clusters[j][y]].push_back(clusters[i][x]);
              }
            }
          }
        }
      }
    }
  }

  void printMatrix() const {
    std::vector<std::vector<int>> matrix;
    matrix.resize(adjList.size(), std::vector<int>(adjList.size()));
    for (size_t i = 0; i < adjList.size(); i++) {
      for (size_t j = i; j < adjList.size(); j++) {
        if (contains1(adjList[i], j)) {
          matrix[i][j] = 1;
          matrix[j][i] = 1;
        } else {
          matrix[i][j] = 0;
          matrix[j][i] = 0;
        }
      }
    }
    for (size_t i = 0; i < matrix.size(); i++) {
      for (size_t j = 0; j < matrix.size(); j++) {
        std::cout << matrix[i][j] << " ";
      }
      std::cout << std::endl;
    }
  }

  void printProbabilities() const {
    std::cout << "p: " << p << std::endl;
    std::cout << "q: " << q << std::endl;
    std::cout << "intraProb: " << intraProb << std::endl;
    std::cout << "interProb: " << interProb << std::endl;
  }
};

#endif // CLUSTEREDGRAPH_H_INCLUDED
