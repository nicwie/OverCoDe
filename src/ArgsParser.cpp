#include "ArgsParser.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

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

    params.isEgoGraph = true;
    params.n = 125;

    double logn = log2(params.n);

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

    params.isEgoGraph = false;
    params.n = 5000;

    double logn = log2(params.n);

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
