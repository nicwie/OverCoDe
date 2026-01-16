#ifndef ARGSPARSER_H
#define ARGSPARSER_H

#include <string>
#include <vector>

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

AppParams parseArgs(int argc, char *argv[]);

#endif // ARGSPARSER_H
