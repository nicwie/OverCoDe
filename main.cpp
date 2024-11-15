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
//#include "DistributedProcess.h"



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

    //string filename = "clusters/test_auto";

    int n = 5000;

    vector<int> overlaps {0};

    if (argc <= 4) {
        cerr << "Not enough arguments! Usage: ./main OutputFile Graphs Runs overlapSize [overlapSize [overlapSize ...]]" << endl;
        return -1;
    }

    string filename = argv[1];

    if (stoi(argv[2]) < 1 || stoi(argv[3]) < 1) {
        cerr << "Graphs and Runs must be >= 1!" << endl;
        return -1;
    }

    int graphs = stoi(argv[2]);
    int runs = stoi(argv[3]);

    for (int i = 4; i < argc; i++) {
        overlaps.push_back(stoi(argv[i]));
    }

    cout << "Running with " << graphs << " graphs and " << runs << " runs, with overlaps:" <<endl;
    bool first = true;
    for (int overlap : overlaps) {
        // this is done so that we do not print the overlap at overlaps[0], which is always 0
        // overlaps[0] is zero because we only calculate the size of the cluster without
        // its overlaps in ClusteredGraph.h
        if (first) {
            first = false;
            continue;
        }
        cout << overlap << " ";
    }
    cout << endl;



    float p = pow((float) n, 0.75) / n;
    int T = (5 * log2(n));   // Number of rounds
    int k = (log2(n) / p);   // Number of pushes
    int rho = 3; // Number of majority samples
    int l = T; // Number of iterations

    double alpha_1 = 0.9; // Threshold value, similarity (0.9)
    double alpha_2 = 0.92; // Majority (history) threshold (0.9 | 0.92)

    ofstream a;
    a.open(filename); // done to delete file contents if file already exists
    a.close();

    cout << "Before graph" << endl;

    for (int i = 0; i < graphs; i++) {
        ClusteredGraph* graph = new ClusteredGraph(n, overlaps);

        // vector<vector<int>> adjList = readGraphFromFile("email-Eu-core.txt");
        cout << "Graph created" << endl;



        for (int j = 0; j < runs; j++) {
            cout << "[" << i << "]" << "[" << j << "]" << endl;

            OverCoDe* ocd = new OverCoDe(graph->getAdjList(), T, k, rho, l, alpha_1, alpha_2);
            //OverCoDe* ocd = new OverCoDe(adjList, T, k, rho, l, alpha_1, alpha_2);
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
            // ocd->printClusters();
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
