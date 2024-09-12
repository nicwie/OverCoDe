#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <string>
#include <algorithm>
#include <numeric>
#include "OverCoDe.h"
#include "ClusteredGraph.h"
#include "DistributedProcess.h"


using namespace std;

int main() {
    int n = 1000;  // Number of nodes in each cluster
    float p = pow((float) n, 0.75) / n;
    float T = (5 * log2(n));   // Number of rounds
    int k = (log(n) / p);   // Number of pushes
    int rho = 3; // Number of majority samples
    int l = T; // Number of iterations
    double alpha_1 = 0.9; // Threshold value
    double alpha_2 = 0.7;

    // Create a ClusteredGraph object
    ClusteredGraph graph(n);

    cout << "Graph created" << endl;

    // Only run dp, not OCD
     // DistributedProcess dp(graph.getAdjList(), T, k, rho);
     // dp.runProcess();

    OverCoDe ocd(graph.getAdjList(), T, k, rho, l, alpha_1, alpha_2);

    ocd.runOverCoDe();

    ocd.printClusters();

    ocd.printClustersToFile("test_" + to_string(graph.getAdjList().size()) + "_nodes.txt");



    // Print resulting matrices to cout
    // graph.printGraph();
    // dp.printStates();

    // Print dp Token Amounts after processes
    // dp.printResults();


    return 0;
}
