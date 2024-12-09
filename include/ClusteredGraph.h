#ifndef CLUSTEREDGRAPH_H_INCLUDED
#define CLUSTEREDGRAPH_H_INCLUDED

#include <iostream>
#include <vector>
#include <cmath>
#include "RandomGenerator.h"
#include "Graph.h"

using namespace std;

inline bool contains1 (const vector<unsigned long long>& g, const int n) {
    return any_of(g.begin(), g.end(), [&n] (const unsigned long long x) { return x == static_cast<unsigned long long>(n); });
}

class ClusteredGraph final : public Graph {
private:
    int n;  // Number of nodes in each cluster
    int clusterNr;
    double p;
    double q;
    double intraProb;  // Probability of intra-cluster edges
    double interProb;  // Probability of inter-cluster edges

    vector<unsigned long long> overlaps; // stores how large the overlap between clusters should be (there is an overlap of size overlaps[n-1] between n clusters)
    RandomGenerator rng;

    bool coinFlip(const double probability) {
        return (rng.getRandomDouble(0.0,100.0) / 100.0) < probability;
    }

    // n Choose r
    static long long C(const int n, int r) {
        if(r > n - r){
            r = n - r; // because C(n, r) == C(n, n - r)
        }
        long long ans = 1;

        for(int i = 1; i <= r; i++) {
            ans *= n - r + i;
            ans /= i;
        }

        return ans;
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
    static void addNodes(vector<vector<unsigned long long>> &clusters, const vector<unsigned long long> &overlaps) {
        unsigned long long usedNodes = 0;

        for(int i = 0; i < static_cast<int>(overlaps.size()); i++) {
            if (i == 0) {
                // add pure (non-overlapping) nodes to clusters
                for (int j = 0; j < static_cast<int>(clusters.size()); j++) {
                    for (unsigned long long m = overlaps[i] * j; m < overlaps[i] * (j + 1); m++) {
                        clusters[j].push_back(m);
                    }
                }
                usedNodes += static_cast<int>(clusters.size()) * overlaps[0];
            }
            else {
                // add overlapping nodes to clusters
                const int r = i+1;
                vector<vector<int>> combinations;

                // all possible combinations of r clusters
                generateCombinations(static_cast<int>(clusters.size()), r, combinations);

                for (unsigned long long m = 0; m < overlaps[i]; m++) {
                    for (const auto &comb : combinations) {
                        for (const int clusterId : comb) {
                            clusters[clusterId].push_back(usedNodes);
                        }
                        usedNodes++;
                    }
                }
            }
        }
    }




public:
    ClusteredGraph(const int numNodes, const vector<unsigned long long>& overlap)
        : Graph(), n(numNodes),
          clusterNr(static_cast<int>(overlap.size())),
          p(2.5 * pow(log2(n) / static_cast<double>(n), 1 / static_cast<double>(4))),
          q(p / 150),
          intraProb(p),
          interProb(q),
          overlaps(overlap)
    {
        if (intraProb > 1) {
            intraProb = 1;
            interProb = static_cast<float>(1) / 150;
        }
        printProbabilities();
    }

    void generateGraph() override {

        long long nExclusive = n;

        // calculate how many nodes not in overlaps exist per cluster
        // example for 3 clusters:
        // int nExclusive = n - (overlap3 + (C(3 - 1, 2 - 1) * overlap2));
        for (int i = 1; i < static_cast<int>(overlaps.size()); i++) {
            nExclusive -= static_cast<int>(overlaps[i]) * C(static_cast<int>(overlaps.size()) - 1, i);
        }


        overlaps[0] = nExclusive;

        unsigned long long uniqueNodes = 0;

        // calculate the total number of nodes
        // example for 3 clusters
        // int uniqueNodes = overlap3 + (C(3, 2) * overlap2) + (3 * nExclusive);
        for(int i = 0; i < static_cast<int>(overlaps.size()); i++) {
            uniqueNodes += C(static_cast<int>(overlaps.size()), i + 1) * overlaps[i];
        }



        adjList.resize(uniqueNodes);

        clusters.resize(clusterNr);

        vector<vector<bool>> connected; // this stores nodes we have connected already, so that we do not connect nodes in the overlap > 1 time
        connected.resize(uniqueNodes,vector<bool>(uniqueNodes));

        addNodes(clusters, overlaps);

        // now connecting clusters internally

        for (auto & cluster : clusters) {
            for (int j = 0; j < static_cast<int>(cluster.size()); j++) {
                for (int m = j + 1; m < static_cast<int>(cluster.size()); m++) {
                    if (!connected[cluster[m]][cluster[j]]) {
                        connected[cluster[j]][cluster[m]] = true;
                        connected[cluster[m]][cluster[j]] = true;
                        if (coinFlip(intraProb)) {
                            adjList[cluster[j]].push_back(cluster[m]);
                            adjList[cluster[m]].push_back(cluster[j]);
                        }
                    }
                }
            }
        }

        // connecting clusters externally as pairs

        for (int i = 0; i < static_cast<int>(clusters.size()); i++) {
            for (int j = i+1; j < static_cast<int>(clusters.size()); j++) {
                for (int x = 0; x < static_cast<int>(clusters[i].size()); x++) {
                    for (int y = 0; y < static_cast<int>(clusters[j].size()); y++) {
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


    void printMatrix() const {
        vector<vector<int>> matrix;
        matrix.resize(static_cast<int>(adjList.size()), vector<int>(adjList.size()));
        for (int i = 0; i < static_cast<int>(adjList.size()); i++) {
            for (int j = i; j < static_cast<int>(adjList.size()); j++) {
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
        for (int i = 0; i < static_cast<int>(matrix.size()); i++) {
            for (int j = 0; j < static_cast<int>(matrix.size()); j++) {
                cout << matrix[i][j] << " ";
            }
            cout << endl;
        }
    }


    void printProbabilities() const {
    	cout << "p: " << p << endl;
    	cout << "q: " <<  q << endl;
    	cout << "intraProb: " << intraProb << endl;
    	cout << "interProb: " << interProb << endl;
    }

};



#endif // CLUSTEREDGRAPH_H_INCLUDED
