#ifndef CLUSTEREDGRAPH_H_INCLUDED
#define CLUSTEREDGRAPH_H_INCLUDED

#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <stdlib.h>
#include <time.h>

using namespace std;

class ClusteredGraph {
private:
    int n;  // Number of nodes in each cluster
    int overlap;  // Number of overlapping nodes
    double intraProb;  // Probability of intra-cluster edges
    double interProb;  // Probability of inter-cluster edges
    vector<vector<int>> adjList;  // Adjacency list for the graph

    bool coinFlip(double probability) {
        return ((double)(rand() % 100) / 100.0) < probability;
    }

public:
    ClusteredGraph(int numNodes)
        : n(numNodes),
          overlap(numNodes / 50),
          intraProb(pow(numNodes, 0.75) / numNodes),
          interProb(pow(numNodes, 0.75) / (20.0 * numNodes)),
          adjList(2 * numNodes - overlap) {

        srand(time(nullptr));
        generateEdges();
    }

    void generateEdges() {
        // Generate intra-cluster edges for Cluster 1
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                if (coinFlip(intraProb)) {
                    adjList[i].push_back(j);
                    adjList[j].push_back(i);
                }
            }
        }

        // Generate intra-cluster edges for Cluster 2
        for (int i = n - overlap; i < n * 2 - overlap; ++i) {
            for (int j = i + 1; j < n * 2 - overlap; ++j) {
                if (coinFlip(intraProb)) {
                    adjList[i].push_back(j);
                    adjList[j].push_back(i);
                }
            }
        }

        // Generate inter-cluster edges between Cluster 1 and Cluster 2
        for (int i = 0; i < n; ++i) {
            for (int j = n - overlap; j < n * 2 - overlap; ++j) {
                if (coinFlip(interProb)) {
                    adjList[i].push_back(j);
                    adjList[j].push_back(i);
                }
            }
        }
    }

    void printGraph() const {
        for (int i = 0; i < (int)adjList.size(); ++i) {
            cout << "Node " << i << " -> ";
            for (int neighbor : adjList[i]) {
                cout << neighbor << " ";
            }
            cout << endl;
        }
    }

    const vector<vector<int>>& getAdjList() const {
        return adjList;
    }

};



#endif // CLUSTEREDGRAPH_H_INCLUDED
