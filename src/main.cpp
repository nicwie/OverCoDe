// Copyright (c) 2025 Nic Sebastian Wiesinger. All Rights Reserved.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ClusteredGraph.h"
#include "Graph.h"
#include "OverCoDe.h"
#include "SyntheticEgoGraph.h"

// #include "DistributedProcess.h"

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

  int n = 0;
  double beta;
  // beta = 0.95; // similarity threshold
  // double beta = 0.85; // For ego graph
  double alpha;
  // alpha = 0.92; // Majority (history) threshold (0.9 | 0.92)

  int T = 0;
  // int T = static_cast<int>(10 * log2(n));   // Number of rounds
  // int T = static_cast<int>(50 * log2(n)); // For ego graph
  int k = 0;   // Number of pushes
  int rho = 3; // Number of majority samples
  int l = 0;
  // int l = T; // Number of iterations
  // int l = static_cast<int>(250 * log2(n)); // For ego graph

  bool isEgoGraph = false;
  int graphs = 0, runs = 0;
  std::string filename;
  std::vector<unsigned long long> overlaps{0};

  try {
    double p = 0;
    if (argc < 7) {
      std::cerr << "Not enough arguments! Usage: ./main <Is ego Graph?: true | "
                   "false> alpha beta OutputFile Graphs Runs overlapSize "
                   "[overlapSize [overlapSize ...]]"
                << std::endl;
      return -1;
    }

    // if (stod(argv[3]) > 1 || stod(argv[3]) <= 0 || stod(argv[4]) > 1 ||
    // stod(argv[4]) <= 0) {
    //     cerr << "Alpha and beta must be between 1 and 0!" << endl;
    //     return -1;
    // }

    alpha = std::stod(argv[2]);
    beta = std::stod(argv[3]);

    if (std::stoi(argv[5]) < 1 || std::stoi(argv[6]) < 1) {
      std::cerr << "Graphs and Runs must be >= 1!" << std::endl;
      return -1;
    }

    filename = static_cast<std::string>(argv[4]);
    graphs = std::stoi(argv[5]);
    runs = std::stoi(argv[6]);

    if (static_cast<std::string>(argv[1]) == "true") {
      if (argc != 7) {
        std::cout << "Usage: ./main true alpha beta OutputFile Graphs Runs"
                  << std::endl;
        return -1;
      }

      isEgoGraph = true;
      n = 125;
      T = static_cast<int>(50 * log2(n));
      l = static_cast<int>(250 * log2(n));
      p = pow(static_cast<double>(n), 0.75) / n;
      k = static_cast<int>(log2(n) / p);

    } else if (static_cast<std::string>(argv[1]) == "false") {
      if (argc <= 7) {
        std::cout << "Usage: ./main false OutputFile Graphs Runs overlapSize "
                     "[overlapSize [overlapSize ...]]"
                  << std::endl;
        return -1;
      }

      for (int i = 7; i < argc; i++) {
        overlaps.push_back(std::stoull(argv[i]));
      }
      isEgoGraph = false;
      n = 5000;
      T = static_cast<int>(10 * log2(n));
      l = T;
      p = pow(static_cast<double>(n), 0.75) / n;
      k = static_cast<int>(log2(n) / p);
    }
  } catch ([[maybe_unused]] std::exception &e) {
    std::cout << "Oh no! " << e.what() << std::endl;
    return -1;
  }

  std::cout << "Running with " << graphs << " graphs and " << runs << " runs."
            << std::endl;
  if (!isEgoGraph) {
    bool first = true;
    std::cout << "Cluster Graph, with overlaps: " << std::endl;
    for (unsigned long long overlap : overlaps) {
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
  a.open(filename); // done to delete file contents if file already exists
  a.close();

  a.open(filename + "_truth");
  a.close();

  std::cout << "Before graph" << std::endl;

  std::unique_ptr<Graph> graph;

  if (isEgoGraph) {
    // graph = make_unique<SyntheticEgoGraph>(); // only cpp14
    // ReSharper disable once CppSmartPointerVsMakeFunction
    graph = std::unique_ptr<Graph>(new SyntheticEgoGraph());
  } else {
    // graph = make_unique<ClusteredGraph>(n, overlaps); // only cpp14
    // ReSharper disable once CppSmartPointerVsMakeFunction
    graph = std::unique_ptr<Graph>(
        new ClusteredGraph(static_cast<size_t>(n), overlaps));
  }

  for (int i = 0; i < graphs; i++) {
    // have "graph" as abstract class and implement as cluster & ego?

    graph->generateGraph();

    std::cout << "Graph created" << std::endl;

    for (int j = 0; j < runs; j++) {
      std::cout << "[" << i << "]" << "[" << j << "]" << std::endl;
      auto *ocd = new OverCoDe(graph->getAdjList(), T, k, rho,
                               static_cast<size_t>(l), beta, alpha);
      ocd->runOverCoDe();

      std::ofstream f;
      f.open(filename, std::ofstream::app);
      int c = 0;

      f << i << " " << j << std::endl;
      a.open(filename + "_truth", std::ofstream::app);
      a << i << " " << j << std::endl;
      a.close();
      graph->appendTruthToFile(filename + "_truth");

      auto clusters = ocd->getClusters();
      // Cannot use structured bindings in cpp11
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
      delete ocd;
    }
    graph->deleteGraph();
  }

  auto elapsedTime = time(nullptr) - startTime;

  std::cout << ((elapsedTime / 60) / 60) << "h " << (elapsedTime / 60) % 60
            << "min " << elapsedTime % 60 << "s" << std::endl;

  return 0;
}
