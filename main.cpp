#include <iostream>
#include <cstdlib>
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

int main(int argc, char *argv[]) {

    /*
    if (argc != 2) {
	cout << "Usage: ./main <Nodes per cluster>" << endl;
	return 0;
    }

    int n = atoi(argv[1]);  // Number of nodes in each cluster

    if (n < 1) {
	cout << "Invalid number of nodes!" << endl;
	return 0;
    }
    */

    int n = 2500;
    float p = pow((float) n, 0.75) / n;
    float T = (5 * log2(n));   // Number of rounds
    int k = (log(n) / p);   // Number of pushes
    int rho = 3; // Number of majority samples
    int l = T; // Number of iterations
    double alpha_1 = 0.9; // Threshold value
    double alpha_2 = 0.7;

    // Create a ClusteredGraph object
    //ClusteredGraph graph(n);

    cout << "Graph created" << endl;

    // Only run dp, not OCD
     // DistributedProcess dp(graph.getAdjList(), T, k, rho);
     // dp.runProcess();

    //OverCoDe ocd(graph.getAdjList(), T, k, rho, l, alpha_1, alpha_2);

    //ocd.runOverCoDe();



    for (int i = 0; i < 5; i++) {
        ClusteredGraph* graph = new ClusteredGraph(n);
        for (int j = 0; j < 20; j++) {
            cout << "[" << i << "]" << "[" << j << "]" << endl;
            ofstream f;

            f.open("test_4950_repeated_graph_fixed", f.app);
            f << i << " " << j << endl;
            OverCoDe* ocd = new OverCoDe(graph->getAdjList(), T, k, rho, l, alpha_1, alpha_2);
            ocd->runOverCoDe();

            auto clusters = ocd->getClusters();
            for (const auto& pair : clusters) {
                for (int num : pair.first) {
                    f << num << " ";
                }
                f << endl;
                for (int num : pair.second) {
                    f << num << " ";
                }
                f << endl;
            }

            f.close();
            delete ocd;
        }
        delete graph;
    }

    // ocd.printClusters();

    // ocd.printClustersToFile("test_" + to_string(graph.getAdjList().size()) + "_nodes.txt");



    // Print resulting matrices to cout
    // graph.printGraph();
    // dp.printStates();

    // Print dp Token Amounts after processes
    // dp.printResults();


    return 0;
}
