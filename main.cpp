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

    bool contains (vector<int> g, int n) {
        for (int x : g) {
            if (x == n) {
                return true;
            }
        }
        return false;
    }

    vector<vector<int>> readGraphFromFile(string filename) {


        vector<vector<int>> adjList;
        ifstream a;
        a.open(filename);
        string read;
        int node1, node2;
        while (a) {
            a >> read;
            node1 = stoi(read);
            a >> read;
            node2 = stoi(read);
            if (node1 >= (int)adjList.size()) {
                adjList.resize(node1 + 1);
            }
            if (node2 >= (int)adjList.size()) {
                adjList.resize(node2 + 1);
            }

            if (contains(adjList[node1], node2) || contains(adjList[node2], node1)) {
                continue;
            }
            adjList[node1].push_back(node2);
            adjList[node2].push_back(node1);
        }
        return adjList;
    }


int main(int argc, char * argv[]) {

    auto startTime = time(NULL);

    string filename = "test";

    int n = 5000;


    float p = pow((float) n, 0.75) / n;
    int T = (5 * log2(n));   // Number of rounds
    int k = (log2(n) / p);   // Number of pushes
    int rho = 3; // Number of majority samples
    int l = T; // Number of iterations

    double alpha_1 = 0.9; // Threshold value, similarity (0.9)
    double alpha_2 = 0.7; // Majority threshold <= 0.84


    ofstream a;
    a.open(filename); // done to delete file contents if file already exists
    a.close();

    cout << "Before graph" << endl;


    for (int i = 0; i < 1; i++) {
        ClusteredGraph* graph = new ClusteredGraph(n, 3);

        // vector<vector<int>> adjList = readGraphFromFile("email-Eu-core.txt");
        cout << "Graph created" << endl;



        for (int j = 0; j < 1; j++) {
            cout << "[" << i << "]" << "[" << j << "]" << endl;

            OverCoDe* ocd = new OverCoDe(graph->getAdjList(), T, k, rho, l, alpha_1, alpha_2);
            // OverCoDe* ocd = new OverCoDe(adjList, T, k, rho, l, alpha_1, alpha_2);
            ocd->runOverCoDe();


            ofstream f;
            f.open(filename, f.app);
            int c = 0;
            f << i << " " << j << endl;

            auto clusters = ocd->getClusters();
            for (const auto& pair : clusters) {
                f << "Cluster " << ++c << ": ";
                for (int num : pair.first) {
                    f << num << " ";
                }
                f << endl;
                for (int num : pair.second) {
                    f << num << " ";
                }
                f << endl;
                f << endl;
            }
            f << endl << endl;

            f.close();
            ocd->printClusters();
            cout << c << " clusters." << endl;
            //ocd->printHistoryToFile(filename + "_signatures");
            delete ocd;
        }
        delete graph;
    }

    auto elapsedTime = time(NULL) - startTime;

    cout << ((elapsedTime / 60) / 60) << "h " << (elapsedTime / 60) % 60 << "min " << elapsedTime % 60 <<  "s" << endl;


    return 0;
}
