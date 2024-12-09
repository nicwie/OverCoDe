#ifndef SYNTHETICEGOGRAPH_H
#define SYNTHETICEGOGRAPH_H

#include <vector>
#include "Graph.h"
#include "RandomGenerator.h"

using namespace std;

class SyntheticEgoGraph final : public Graph {
    private:
     // stores which nodes are in which cluster
    RandomGenerator rng;

    void addNodes() {
        // counter for where we are (what node we are considering)
        int usedNodes = 0;

        // add pure (non-overlapping) nodes to clusters
        for (auto & cluster : clusters) {

            const int clusterSize = rng.getNormalInt(125, 25);
            for (int j = usedNodes; j < (usedNodes + clusterSize); j++) {
                cluster.push_back(j);
            }
            usedNodes += clusterSize;
        }

        // add overlapping nodes to clusters
        /*
        // This if we expect 1-10 overlap between any(!) possible combination (except all)
        vector<vector<int>> combinations;
        for (int i = 1; i < ((int)clusters.size() - 1); i++) {
            int overlapSize = rng.getRandomInt(0, 10);
            int r = i+1;
            // all possible combinations of r clusters
            generateCombinations(int(clusters.size()), r, combinations);

            for (int m = 0; m < overlapSize; m++) {
                for (const auto &comb : combinations) {
                    for (int clusterId : comb) {
                        clusters[clusterId].push_back(usedNodes);
                    }
                    usedNodes++;
                }
            }
        }
        */

        // this if we expect 1-10 overlap between any 2 clusters


        // all possible combinations of r clusters
        for (int i = 0; i < static_cast<int>(clusters.size()); i++) {
            for (int j = i + 1; j < static_cast<int>(clusters.size()); j++) {
                const int overlapSize = rng.getRandomInt(0, 10);
                for (int m = usedNodes; m < (usedNodes + overlapSize); m++) {
                    clusters[i].push_back(m);
                    clusters[j].push_back(m);
                }
                usedNodes += overlapSize;
            }
        }

        // add ego Node once to a ll clusters
        for (vector<unsigned long long> &cluster : clusters) {
            cluster.push_back(usedNodes);
        }

    }

    public:
        SyntheticEgoGraph() = default;


    void generateGraph() override {
        clusters.resize(rng.getRandomInt(4, 6));
        addNodes();

        int graphSize = 0;

        for (const auto& cluster : clusters) {
            graphSize += static_cast<int>(cluster.size());
        }

        adjList.resize(graphSize);

        vector<vector<bool>> connected; // this stores nodes we have connected already, so that we do not connect nodes in the overlap > 1 time
        connected.resize(graphSize,vector<bool>(graphSize));

        // completely connect clusters internally

        for (auto & cluster : clusters) {
            for (int j = 0; j < static_cast<int>(cluster.size()); j++) {
                for (int m = j + 1; m < static_cast<int>(cluster.size()); m++) {
                    if (!connected[cluster[m]][cluster[j]]) {
                        connected[cluster[j]][cluster[m]] = true;
                        connected[cluster[m]][cluster[j]] = true;
                        adjList[cluster[j]].push_back(cluster[m]);
                        adjList[cluster[m]].push_back(cluster[j]);
                    }
                }
            }
        }


        // add 10-20 edges between any 2 pairs of clusters
        for (int i = 0; i < static_cast<int>(clusters.size()); i++) {
            for (int j = i + 1; j < static_cast<int>(clusters.size()); j++) {
                const int edgeCount = rng.getRandomInt(10, 20);
                for (int m = 0; m < edgeCount; m++) {
                    unsigned long long addToI = clusters[i][rng.getRandomUll(0, static_cast<int>(clusters[i].size()) - 1)];
                    // ReSharper disable once CppTooWideScopeInitStatement
                    unsigned long long addToJ = clusters[j][rng.getRandomUll(0, static_cast<int>(clusters[j].size()) - 1)];

                    if (!connected[addToI][addToJ]) {
                        connected[addToI][addToJ] = true;
                        connected[addToJ][addToI] = true;
                        adjList[addToI].push_back(addToJ);
                        adjList[addToJ].push_back(addToI);
                    }

                }
            }
        }
    }

    [[nodiscard]] vector<vector<unsigned long long>> getClusters() const {
        return clusters;
    }

};

#endif // SYNTHETICEGOGRAPH_H
