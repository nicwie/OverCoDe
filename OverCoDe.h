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
//#include "DistributedProcess.h"
#include "RandomGenerator.h"
#include <omp.h>
#include <thread>
#include <future>
#include <semaphore> // C++20, could be replaced with a <mutex> and <condition_variable> for c++11


using namespace std;

// Define the types of tokens
enum Token { R, B };

class OverCoDe {
private:
	vector<vector<int>> G;
    int T, k, rho, ell;
    double alpha1, alpha2;
    time_t startTime, elapsedTime;
    vector<vector<int>> si;
    vector<vector<vector<int>>> C;
    RandomGenerator rng;

    int maxThreads = thread::hardware_concurrency(); // this is 16 (on my machine)

    struct vectorHash {                             // this seems to not be comaptible with c++11 in some way
    size_t operator()(const vector<int>& v) const {
        size_t seed = v.size();
        for(auto& i : v) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
    };




    // Function to randomly initialize the tokens
    Token randomToken() {
        return ((Token)rng.getRandomInt(0,1));
    }

    // Function to sample k neighbors from a set of neighbors N(u)
    vector<int> sample(int num, const vector<int>& neighbors) {
        vector<int> sampled;
        if (neighbors.size() == 0) {
            return sampled;
        }
        for (int i = 0; i < num ; i++) {
            sampled.push_back(neighbors[rng.getRandomInt(0, (int)neighbors.size() - 1)]);
        }
        return sampled;
    }

    // Function to execute the distributed process
    vector<vector<Token>> distributedProcess(vector<vector<int>>& graph, int T, int k, int rho) {
        int n = (int)graph.size();
        vector<unordered_map<Token, int>> receivedToken;
        receivedToken.resize(n);
        vector<vector<Token>> X;
        X.resize(n, vector<Token> (T+1));

        // Random Initialization
        for (int u = 0; u < n; u++) {
            X[u][0] = (randomToken());
        }

        // Symmetry Breaking
        for (int u = 0; u < n; u++) {
            vector<int> M = sample(k, graph[u]);
            for (int v : M) {
                ++receivedToken[v][X[u][0]];
            }
        }

        for (int u = 0; u < n; u++) {
            // Save the most common state to matrix; if equal, randomize state
            X[u][1] = ((receivedToken[u][R] > receivedToken[u][B]) ? R : (receivedToken[u][R] < receivedToken[u][B]) ? B : randomToken());
        }

        // ρ-Majority process
        for (int t = 2; t <= T; t++) {
            for (int u = 0; u < n; u++) {
                vector<int> v_sampled = sample(rho, G[u]);
                int countB = 0;
                int countR = 0;
                for (int v : v_sampled) {
                    (X[v][t-1] == R) ? countR++ : countB++;
                }
                X[u][t] = ((countR > countB) ? R : (countR < countB) ? B : randomToken());
            }
        }
        return X;
    }

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
        : G(adjList), T(rounds), k(pushes), rho(majoritySamples), ell(L), alpha1(alpha1), alpha2(alpha2), rng() {
        si.resize(G.size());
        C.resize(G.size());
    }

    void runOverCoDe() {

        startTime = time(NULL);

        // Generate Signatures
        // this provides a vector which has the most common result for every node in each iteration
        si.resize(G.size(), vector<int> (ell));

        // Stores vector of promises for the result of dp
        vector<future<vector<vector<Token>>>> threads(ell);

        counting_semaphore<> semaphore(maxThreads);

        // creates threads
        for (int i = 0; i < ell; i++) {
            semaphore.acquire();

            // lambda that releases the semaphore after it dp finishes
            threads[i] = async(launch::async, [this, &semaphore]() {
                auto result = distributedProcess(this->G, this->T, this->k, this->rho);
                semaphore.release();
                return result;
            });
        }

        // waits for all threads to finish
        vector<vector<vector<Token>>> X(ell + 1);
        for (int i = 0; i < ell; i++) {
             X[i] = threads[i].get();
        }

        // Count if a state appers more often than alpha2 * T
        for (int i = 0; i < ell; i++) {
            for (int u = 0; u < (int)G.size(); ++u) {
                int countR = 0, countB = 0;
                for (int t = 0; t < (int)X[i][u].size(); t++) {
                    (X[i][u][t] == R) ? countR++ : countB++;
                }

                if (countR >= alpha2 * T) {
                    si[u].push_back(R);
                } else if (countB >= alpha2 * T) {
                    si[u].push_back(B);
                } else {
                    si[u].push_back(-1); // Assume -1 indicates uncertainty
                }
            }
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
