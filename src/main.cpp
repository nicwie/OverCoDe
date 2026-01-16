#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ArgsParser.h"
#include "ClusteredGraph.h"
#include "Graph.h"
#include "OverCoDe.h"
#include "SyntheticEgoGraph.h"

/**
 * @brief Checks if an int vector contains a number.
 * A wrapper around std::any_of
 *
 * @param[in] g The vector to check
 * @param[in] n The number to check for
 * @return True if it is contained, false if not
 */
bool contains(const std::vector<int> &g, const size_t n) {
  return std::any_of(g.begin(), g.end(),
                     [&n](const size_t i) { return i == n; });
}

std::vector<std::vector<int>> readGraphFromFile(const std::string &filename) {
  std::vector<std::vector<int>> adjList;
  std::ifstream a;
  a.open(filename);
  std::string read;
  while (a) {
    a >> read;
    size_t node1 = std::stoull(read);
    a >> read;
    size_t node2 = stoull(read);
    if (node1 >= adjList.size()) {
      adjList.resize(node1 + 1);
    }
    if (node2 >= adjList.size()) {
      adjList.resize(node2 + 1);
    }

    if (contains(adjList[node1], node2) || contains(adjList[node2], node1)) {
      continue;
    }
    adjList[node1].push_back(static_cast<int>(node2));
    adjList[node2].push_back(static_cast<int>(node1));
  }
  return adjList;
}

int main(int argc, char *argv[]) {
  auto startTime = time(nullptr);

  // beta = 0.95; // similarity threshold
  // beta = 0.85; // For ego graph
  // alpha = 0.92; // Majority (history) threshold (0.9 | 0.92)

  AppParams params;
  try {
    params = parseArgs(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << "Error parsing arguments: " << e.what() << std::endl;
    return -1;
  }

  std::cout << "Running with " << params.graphs << " graphs and " << params.runs
            << " runs." << std::endl;
  if (!params.isEgoGraph) {
    bool first = true;
    std::cout << "Cluster Graph, with overlaps: " << std::endl;
    for (unsigned long long overlap : params.overlaps) {
      // this is done so that we do not print the overlap at overlaps[0], which
      // is always 0 overlaps[0] is zero because we calculate the size of the
      // cluster without its overlaps in ClusteredGraph.h
      if (first) {
        first = false;
        continue;
      }
      std::cout << overlap << " ";
    }
    std::cout << std::endl;
  } else {
    std::cout << "Ego graph." << std::endl;
  }

  std::ofstream a;
  a.open(
      params.filename); // done to delete file contents if file already exists
  a.close();

  a.open(params.filename + "_truth");
  a.close();

  std::cout << "Before graph" << std::endl;

  std::unique_ptr<Graph> graph;

  if (params.isEgoGraph) {
    graph = std::unique_ptr<Graph>(new SyntheticEgoGraph());
  } else {
    graph = std::unique_ptr<Graph>(
        new ClusteredGraph(static_cast<size_t>(params.n), params.overlaps));
  }

  for (int i = 0; i < params.graphs; i++) {
    graph->generateGraph();

    std::cout << "Graph created" << std::endl;

    for (int j = 0; j < params.runs; j++) {
      std::cout << "[" << i << "]" << "[" << j << "]" << std::endl;
      OverCoDe ocd(graph->getAdjList(), params.T, params.k, params.rho,
                   params.h, static_cast<size_t>(params.l), params.beta,
                   params.alpha);
      ocd.runOverCoDe();

      std::ofstream f;
      f.open(params.filename, std::ofstream::app);
      int c = 0;

      f << i << " " << j << std::endl;
      a.open(params.filename + "_truth", std::ofstream::app);
      a << i << " " << j << std::endl;
      a.close();
      graph->appendTruthToFile(params.filename + "_truth");

      auto clusters = ocd.getClusters();
      for (const auto &cluster : clusters) {
        f << "Cluster " << ++c << ": ";
        for (int num : cluster.first) {
          f << num << " ";
        }
        f << std::endl;
        for (int num : cluster.second) {
          f << num << " ";
        }
        f << std::endl;
        f << std::endl;
      }
      f << std::endl << std::endl;
      f.close();
      std::cout << c << " clusters." << std::endl;
    }
    graph->deleteGraph();
  }

  auto elapsedTime = time(nullptr) - startTime;

  std::cout << ((elapsedTime / 60) / 60) << "h " << (elapsedTime / 60) % 60
            << "min " << elapsedTime % 60 << "s" << std::endl;

  return 0;
}
