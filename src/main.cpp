#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ClusteredGraph.h"
#include "Graph.h"
#include "OverCoDe.h"
#include "SyntheticEgoGraph.h"

struct AppParams {
  bool isEgoGraph = false;
  double alpha = 0.0;
  double beta = 0.0;
  std::string filename;
  int graphs = 0;
  int runs = 0;
  std::vector<unsigned long long> overlaps{0};

  // derived
  int n = 0;
  int T = 0;   // rounds
  int k = 0;   // pushes
  int l = 0;   // iterations
  int rho = 3; // majority samples
  int h = 0;   // sampling neighbors
};

AppParams parseArgs(int argc, char *argv[]) {
  AppParams params;

  if (argc < 7) {
    throw std::runtime_error(
        "Not enough Arguments! Usage: ./OverCoDe <true|false> alpha beta "
        "OutputFile Graphs Runs overlapSize [overlapSize ...]");
  }

  if (std::stod(argv[2]) > 1 || std::stod(argv[2]) <= 0 ||
      std::stod(argv[3]) > 1 || std::stod(argv[3]) <= 0) {
    throw std::runtime_error("Alpha and beta must be between 1 and 0!");
  }

  params.alpha = std::stod(argv[2]);
  params.beta = std::stod(argv[3]);

  if (std::stoi(argv[5]) < 1 || std::stoi(argv[6]) < 1) {
    throw std::runtime_error("Graphs and Runs must be >= 1!");
  }

  params.filename = static_cast<std::string>(argv[4]);
  params.graphs = std::stoi(argv[5]);
  params.runs = std::stoi(argv[6]);

  if (static_cast<std::string>(argv[1]) == "true") {
    if (argc != 7) {
      throw std::runtime_error(
          "Usage: ./main true alpha beta OutputFile Graphs Runs");
    }

    double logn = log2(params.n);
    params.isEgoGraph = true;
    params.n = 125;
    params.T = static_cast<int>(50 * logn);
    params.l = static_cast<int>(250 * logn);
    int c = 2; // two or three
    params.k = static_cast<int>(c * sqrt(params.n) * logn);
    params.h = static_cast<int>(c * sqrt(params.n));

  } else if (static_cast<std::string>(argv[1]) == "false") {
    if (argc <= 7) {
      throw std::runtime_error(
          "Usage: ./main false Alpha Beta OutputFile Graphs Runs overlapSize "
          "[overlapSize [overlapSize ...]]");
    }

    for (int i = 7; i < argc; i++) {
      params.overlaps.push_back(std::stoull(argv[i]));
    }
    double logn = log2(params.n);
    params.isEgoGraph = false;
    params.n = 5000;
    params.T = static_cast<int>(10 * logn);
    params.l = params.T;
    int c = 2; // two or three
    params.k = static_cast<int>(c * sqrt(params.n) * logn);
    params.h = static_cast<int>(c * sqrt(params.n));
  }
  std::cout << "Calculated parameters:" << std::endl;
  std::cout << "  n (nodes): " << params.n << std::endl;
  std::cout << "  T (rounds): " << params.T << std::endl;
  std::cout << "  l (runs): " << params.l << std::endl;
  std::cout << "  k (pushes): " << params.k << std::endl;
  std::cout << "  h (samples): " << params.h << std::endl;

  return params;
}

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
