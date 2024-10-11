#ifndef CLUSTEREDGRAPH_H_INCLUDED
#define CLUSTEREDGRAPH_H_INCLUDED

#include <iostream>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <stdlib.h>
#include <algorithm>
#include "RandomGenerator.h"

using namespace std;

    bool contains1 (vector<int> g, int n) {
        for (int x : g) {
            if (x == n) {
                return true;
            }
        }
        return false;
    }

class ClusteredGraph {
private:
    int n;  // Number of nodes in each cluster
    int clusterNr;
    int overlap;  // Number of overlapping nodes
    double p;
    double q;
    double intraProb;  // Probability of intra-cluster edges
    double interProb;  // Probability of inter-cluster edges
    vector<vector<int>> adjList;  // Adjacency list for the graph
    RandomGenerator rng;

    bool coinFlip(double probability) {
        return (rng.getRandomDouble(0.0,100.0) / 100.0) < probability;
    }

    // n Choose r
    long long C(int n, int r) {
        if(r > n - r){
            r = n - r; // because C(n, r) == C(n, n - r)
        }
        long long ans = 1;
        int i;

        for(i = 1; i <= r; i++) {
            ans *= n - r + i;
            ans /= i;
        }

        return ans;
    }


    // Helper function to generate combinations of size r from n clusters
    void generateCombinations(int n, int r, vector<vector<int>>& combinations) {
        vector<bool> v(n);
        fill(v.begin(), v.begin() + r, true); // First r elements are true (included in combination)

        do {
            vector<int> comb;
            for (int i = 0; i < n; ++i) {
                if (v[i]) {
                    comb.push_back(i); // Add cluster index to combination
                }
            }

            combinations.push_back(comb); // Add combination of clusters

        } while (prev_permutation(v.begin(), v.end()));
    }

    // adds node to clusters according to overlaps
    /* thinking for 3 clusters
        first we need to add overlap[0] (unique nodes) to each cluster
        then we need to add overlap[1] to all clusters as pairs 3nCr2 times
        then we need to add overlap[2] to all clusters at once
        so
        1 1 1
        2 2 2
        3
        nCr times

        for 4 we need
        1 1 1 1
        2 2 2 2 2 2
        3 3 3 3
        4
    */
    void addNodes(vector<vector<int>> &clusters, vector<signed int> &overlaps) {
        int usedNodes = 0;

        for(int i = 0; i < int(overlaps.size()); i++) {
            if (i == 0) {
                // add pure (non-overlapping) nodes to clusters
                for (int j = 0; j < int(clusters.size()); j++) {
                    for (int m = overlaps[i] * j; m < overlaps[i] * (j + 1); m++) {
                        clusters[j].push_back(m);
                    }
                }
                usedNodes += clusters.size() * overlaps[0];
            }
            else {
                // add overlapping nodes to clusters
                int r = i+1;
                vector<vector<int>> combinations;

                // all possible combinations of r clusters
                generateCombinations(int(clusters.size()), r, combinations);

                for (int m = 0; m < overlaps[i]; m++) {
                    for (const auto &comb : combinations) {
                        for (int clusterId : comb) {
                            clusters[clusterId].push_back(usedNodes);
                        }
                        usedNodes++;
                    }
                }
            }
        }
    }




public:
    ClusteredGraph(int numNodes, int clusters)
        : n(numNodes),
          clusterNr(clusters),
          overlap(numNodes / 50),
          p(2.5 * ((log2(n)) / pow(n, (double)1/4))),
          q(p / 60),
          intraProb(p),
          interProb(q),
          rng()
    {
        if(intraProb > 1) {
            intraProb = 1;
            interProb = (float)1 / 60;
        }
        printProbabilities();
        generateClusters();
    }

    void generateClusters() {

        vector<signed int> overlaps; // stores how large the overlap between clusters should be (there is an overlap of size overlaps[n-1] between n clusters)
        overlaps.resize(clusterNr);

        overlaps[1] = 30;
        overlaps[2] = 10;
        //overlaps[3] = 1;

        int nExclusive = n;

        // calculate how many nodes not in overlaps exist per cluster
        // example for 3 clusters:
        // int nExclusive = n - (overlap3 + (C(3 - 1, 2 - 1) * overlap2));
        for (int i = 1; i < int(overlaps.size()); i++) {
            nExclusive -= overlaps[i] * C((int(overlaps.size())) - 1, i);
        }


        overlaps[0] = nExclusive;

        int uniqueNodes = 0;

        // calculate the total number of nodes
        // example for 3 clusters
        // int uniqueNodes = overlap3 + (C(3, 2) * overlap2) + (3 * nExclusive);
        for(int i = 0; i < int(overlaps.size()); i++) {
            uniqueNodes += C(int(overlaps.size()), i + 1) * overlaps[i];
        }



        adjList.resize(uniqueNodes);

        vector<vector<int>> clusters;
        clusters.resize(clusterNr);

        vector<vector<bool>> connected; // this stores nodes we have connected already, so that we do not connect nodes in the overlap > 1 time
        connected.resize(uniqueNodes,vector<bool>(uniqueNodes));

        addNodes(clusters, overlaps);

        // now connecting clusters internally

        for (int i = 0; i < int(clusters.size()); i++) {
            for (int j = 0; j < int(clusters[i].size()); j++) {
                for (int m = j + 1; m < int(clusters[i].size()); m++) {
                    if (!connected[clusters[i][m]][clusters[i][j]]) {
                        connected[clusters[i][j]][clusters[i][m]] = true;
                        connected[clusters[i][m]][clusters[i][j]] = true;
                        if (coinFlip(intraProb)) {
                            adjList[clusters[i][j]].push_back(clusters[i][m]);
                            adjList[clusters[i][m]].push_back(clusters[i][j]);
                        }
                    }
                }
            }
        }

        // connecting clusters externally as pairs

        for (int i = 0; i < int(clusters.size()); i++) {
            for (int j = i+1; j < int(clusters.size()); j++) {
                for (int x = 0; x < int(clusters[i].size()); x++) {
                    for (int y = 0; y < int(clusters[j].size()); y++) {
                        if (!connected[clusters[j][y]][clusters[i][x]]) {
                            connected[clusters[i][x]][clusters[j][y]] = true;
                            connected[clusters[j][y]][clusters[i][x]] = true;
                            if (coinFlip(interProb)) {
                                adjList[clusters[i][x]].push_back(clusters[j][y]);
                                adjList[clusters[j][y]].push_back(clusters[i][x]);
                            }
                        }
                    }
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



    void printMatrix() const {
        vector<vector<int>> matrix;
        matrix.resize(int(adjList.size()), vector<int>(adjList.size()));
        for (int i = 0; i < int(adjList.size()); i++) {
            for (int j = i; j < (int)adjList.size(); j++) {
                if(contains1(adjList[i], j)) {
                    matrix[i][j] = 1;
                    matrix[j][i] = 1;
                }
                else {
                    matrix[i][j] = 0;
                    matrix[j][i] = 0;
                }
            }
        }
        for (int i = 0; i < (int)matrix.size(); i++) {
            for (int j = 0; j < (int)matrix.size(); j++) {
                cout << matrix[i][j] << " ";
            }
            cout << endl;
        }
    }

    const vector<vector<int>>& getAdjList() const {
        return adjList;
    }
    
    void printProbabilities() {
    	cout << "p: " << p << endl;
    	cout << "q: " <<  q << endl;
    	cout << "intraProb: " << intraProb << endl;
    	cout << "interProb: " << interProb << endl;
    }

};



#endif // CLUSTEREDGRAPH_H_INCLUDED
