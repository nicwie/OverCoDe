#ifndef OVERCODE_H_INCLUDED
#define OVERCODE_H_INCLUDED


#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <functional>
#include <stdlib.h>
#include <time.h>
#include "DistributedProcess.h"
#include <omp.h>


using namespace std;


class OverCoDe {
private:
	vector<vector<int>> G;
    int T, k, rho, ell;
    double alpha1, alpha2;
    time_t startTime, elapsedTime;
    vector<vector<int>> si;
    vector<vector<vector<int>>> C;

    struct vectorHash {
    size_t operator()(const vector<int>& v) const {
        size_t seed = v.size();
        for(auto& i : v) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
    };

    /**
     * goal: if we have
     * vec1: R B B U R U B
     * vec2: B B R U R B B
     * valid:0 1 0 0 1 0 1
     * we would get (3 / 7)
     * @return Similarity between signatures
     */
    double similarity(const vector<int>& a, const vector<int>& b) {
        int common = 0, validIndices = 0;

        for (int i = 0; i < (int)min(a.size(), b.size()); i++) {
            if (a[i] != -1 && b[i] != -1) {  // Ensure both indices are valid
                validIndices++;
                if (a[i] == b[i]) {
                    common++;
                }
            }
        }

        if (validIndices == 0) {
            return 0.0;  // Avoid division by zero if there are no valid indices
        }

        return (double)common / validIndices;  // Ratio of matching valid states
    }

    vector<vector<int>> clustersIDs (vector<vector<int>>& S, double alpha) {
        vector<vector<int>> signatures;
        for (const auto& signatureV : S) {
            bool isUnique = true;
            for (const auto& signatureU : signatures) {
                if (similarity(signatureU, signatureV) >= alpha) {
                    isUnique = false;
                    break;
                }
            }
            if (isUnique) {
                signatures.push_back(signatureV);
            }
        }
        return signatures;
    }


public:
    OverCoDe(vector<vector<int>> adjList, int rounds, int pushes, int majoritySamples, int L, double alpha1, double alpha2)
        : G(adjList), T(rounds), k(pushes), rho(majoritySamples), ell(L), alpha1(alpha1), alpha2(alpha2) {
        si.resize(G.size());
        C.resize(G.size());
    }

    void runOverCoDe() {

        startTime = time(NULL);

        // Generate Signatures
        // this loop provides a vector which has the most common result for every node in each iteration
        for (int i = 1; i <= ell; ++i) {
            DistributedProcess* dp = new DistributedProcess(G, T, k, rho);
            dp->runProcess();
            // cout << i << " of " << ell << endl;
            // dp->printResults();
            const vector<vector<Token>>& X = dp->getResult();
            // Count if a state appers more often than alpha2 * T
            for (int u = 0; u < (int)G.size(); ++u) {
                int countR = 0, countB = 0;
                for (int t = 0; t < (int)X[u].size(); t++) {
                    (X[u][t] == R) ? countR++ : countB++;
                }

                if (countR >= alpha2 * T) {
                    si[u].push_back(R);
                } else if (countB >= alpha2 * T) {
                    si[u].push_back(B);
                } else {
                    si[u].push_back(-1); // Assume -1 indicates uncertainty
                }
            }
            delete dp;
        }

        cout << "Done generating signatures" << endl;


        vector<vector<int>> sharedMemory(G.size());

        // Identify Clusters
        for (int u = 0; u < (int)G.size(); ++u) {
            //if (bernoulli(log((double)G.size()) / (double)G.size())) {
                if (count_if(si[u].begin(), si[u].end(), [](int x) {return x != -1; }) >= alpha1 * ell) {
                    sharedMemory[u] =  si[u];
                }
            //}
        }

        vector<vector<int>> signatures = clustersIDs(sharedMemory, alpha1);

        cout << "Got Pure Signatures" << endl;
        // #pragma omp parallel for
        for (int u = 0; u < (int)G.size(); ++u) {
            for (vector<int> signature : signatures) {
                if (similarity(si[u], signature) >= alpha1) {
                    // #pragma omp critical
                    C[u].push_back(signature);
                }
            }
        }

        elapsedTime = time(NULL) - startTime; // used in printClustersToFile
    }

    const vector<vector<vector<int>>>& getResults() const {
        return C;
    }

    void printResults() {
        for (int i = 0; i < (int)C.size(); i++) {
            cout << "Clusters node " << i << " is in: " << endl;
            for (int j = 0; j < (int)C[i].size(); j++) {
                for (int x = 0; x < (int)C[i][j].size(); x++) {
                    cout << C[i][j][x] << " ";
                }
                cout << " / ";
            }
            cout << endl;
        }
    }

    void printHistoryToFile(string filename) {
        ofstream c;
        c.open(filename);
        int i = 0;

        for (vector<int>& vec : si) {
            c << i << endl;
            // cout << i << endl;
            i++;
            for(int& node : vec) {
                c << node << " ";
                // cout << node << " ";
            }
            c << endl << endl;
            // cout << endl << endl;
        }

        c.close();
        cout << "history written" << endl;
    }

    unordered_map<vector<int>, unordered_set<int>, vectorHash> getClusters() {
        unordered_map<vector<int>, unordered_set<int>, vectorHash> clusters;
        for (int i = 0; i < (int)C.size(); i++) {
            for (int j = 0; j < (int)C[i].size(); j++) {
                clusters[C[i][j]].insert(i);
            }
        }
        return clusters;
    }

    void printClusters() {
        auto clusters = getClusters();
        int i = 0;
        for (const auto& pair : clusters) {
            cout << "Cluster "  << ++i << ": ";
            for (int num : pair.first) {
                cout << num << " ";
            }
            cout << endl << "Nodes: ";
            for (int num : pair.second) {
                cout << num << " ";
            }
            cout << endl;
        }
    }

    void printClustersToFile(string name) {
        ofstream f;
        f.open(name);
        int i = 0;
        auto clusters = getClusters();
        for (const auto& pair : clusters) {
            f << "Cluster " << ++i << ": ";
            for (int num : pair.first) {
                f << num << " ";
            }
            f << endl << "Nodes: ";
            for (int num : pair.second) {
                f << num << " ";
            }
            f << endl;
        }
        f << "Time taken: ~" << elapsedTime <<  "s" << endl;
        f.close();
    }

};



#endif // OVERCODE_H_INCLUDED
