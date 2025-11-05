#ifndef GRAPH_H
#define GRAPH_H

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Graph {
public:
  Graph() = default;
  virtual ~Graph() = default;

  virtual void generateGraph() = 0;

  const std::vector<std::vector<unsigned long long>> &getAdjList() const {
    return adjList;
  }

  void printGraph() const {
    for (size_t i = 0; i < adjList.size(); ++i) {
      std::cout << "Node " << i << " -> ";
      for (const unsigned long long neighbor : adjList[i]) {
        std::cout << neighbor << " ";
      }
      std::cout << std::endl;
    }
  }

  void appendTruthToFile(const std::string &filename) const {
    std::cout << "Writing truth to file '" << filename << "'!" << std::endl;
    std::ofstream f;
    f.open(filename, std::ofstream::app);
    if ((f.rdstate() & std::ifstream::failbit) != 0) {
      f.close();
      std::cerr << "Error writing truth to file: '" << filename << "'!"
                << std::endl;
      return;
    }
    for (size_t i = 0; i < clusters.size(); ++i) {
      f << "Cluster " << i + 1 << ": " << std::endl;
      for (const unsigned long long neighbor : clusters[i]) {
        f << neighbor << " ";
      }
      f << std::endl;
    }
    f.close();
  }

  void deleteGraph() {
    adjList.clear();
    clusters.clear();
  }

protected:
  std::vector<std::vector<unsigned long long>>
      adjList; // Adjacency list for the graph
  std::vector<std::vector<unsigned long long>> clusters{0};

  // Helper function to generate combinations of size r from n clusters
  static void
  generateCombinations(const unsigned long long n, const unsigned long long r,
                       std::vector<std::vector<int>> &combinations) {
    std::vector<bool> v(n);
    fill_n(v.begin(), r,
           true); // First r elements are true (included in combination)

    do {
      std::vector<int> comb;
      for (unsigned long long i = 0; i < n; ++i) {
        if (v[i]) {
          comb.push_back(
              static_cast<int>(i)); // Add cluster index to combination
        }
      }

      combinations.push_back(comb); // Add combination of clusters
    } while (std::prev_permutation(v.begin(), v.end()));
  }
};

#endif // GRAPH_H
