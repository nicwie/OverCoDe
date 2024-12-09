#ifndef OVERCODE_H_INCLUDED
#define OVERCODE_H_INCLUDED


#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <ctime>
//#include "DistributedProcess.h"
#include "RandomGenerator.h"
#include <thread>
#include <future>
// #include <semaphore> // C++20, could be replaced with a <mutex> and <condition_variable> for c++11


using namespace std;

// Define the types of tokens
enum Token { R, B };

namespace std {
    template <>
        struct hash<Token> {
        size_t operator()(const Token& token) const noexcept {
            return static_cast<size_t>(token);
        }
    };
}

class OverCoDe {
private:
    vector<vector<unsigned long long>> G;
    int T, k, rho, ell;
    double beta, alpha;
    time_t startTime{}, elapsedTime{};
    vector<vector<int>> si;
    vector<vector<vector<int>>> C;
    RandomGenerator rng;

    //int maxThreads = thread::hardware_concurrency(); // this is 16 (on my machine)

    int maxThreads = 12;

    struct vectorHash {                             // this seems to not be compatible with c++11 in some way
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
        return static_cast<Token>(rng.getRandomInt(0, 1));
    }

    // Function to sample k neighbors from a set of neighbors N(u)
    vector<unsigned long long> sample(const int num, const vector<unsigned long long>& neighbors) {
        vector<unsigned long long> sampled;
        if (neighbors.empty()) {
            return sampled;
        }
        for (int i = 0; i < num ; i++) {
            sampled.push_back(neighbors[rng.getRandomInt(0, static_cast<int>(neighbors.size()) - 1)]);
        }
        return sampled;
    }

    // Function to execute the distributed process
    vector<vector<Token>> distributedProcess(const vector<vector<unsigned long long>> &graph, const int T, const int k, const int rho) {
        const int n = static_cast<int>(graph.size());
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
            vector<unsigned long long> M = sample(k, graph[u]);
            for (const unsigned long long v : M) {
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
                vector<unsigned long long> v_sampled = sample(rho, G[u]);
                int countB = 0;
                int countR = 0;
                for (const unsigned long long v : v_sampled) {
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
    static double similarity(const vector<int>& a, const vector<int>& b) {
        int common = 0, validIndices = 0;

        for (int i = 0; i < static_cast<int>(min(a.size(), b.size())); i++) {
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

        return static_cast<double>(common) / validIndices;  // Ratio of matching valid states
    }

    static vector<vector<int>> clustersIDs (vector<vector<int>>& S, const double alpha) {
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
    OverCoDe(const vector<vector<unsigned long long>>& adjList, const int rounds, const int pushes, const int majoritySamples, const int L, const double beta, const double alpha)
        : G(adjList), T(rounds), k(pushes), rho(majoritySamples), ell(L), beta(beta), alpha(alpha) {
        si.resize(G.size());
        C.resize(G.size());
    }

    void runOverCoDe() {

        startTime = time(nullptr);

        // Generate Signatures
        // this provides a vector which has the most common result for every node in each iteration
        si.resize(G.size(), vector<int> (ell));



        /*
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

        // Count if a state appears more often than alpha2 * T
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
        */

        for (int i = 1; i <= ell; ++i) {

            const vector<vector<Token>>& X = distributedProcess(G, T, k, rho);
            // Count if a state appears more often than alpha2 * T
            for (int u = 0; u < static_cast<int>(G.size()); ++u) {
                int countR = 0, countB = 0;
                for (const auto t : X[u]) {
                    (t == R) ? countR++ : countB++;
                }

                if (countR >= alpha * T) {
                    si[u].push_back(R);
                } else if (countB >= alpha * T) {
                    si[u].push_back(B);
                } else {
                    si[u].push_back(-1); // Assume -1 indicates uncertainty
                }
            }
        }


        cout << "Done generating signatures" << endl;


        vector<vector<int>> sharedMemory(G.size());

        // Identify Clusters
        for (int u = 0; u < static_cast<int>(G.size()); ++u) {
            //if (bernoulli(log((double)G.size()) / (double)G.size())) {
                if (static_cast<double>(count_if(si[u].begin(), si[u].end(), [](const int x) {return x != -1; })) >= beta * ell) {
                    sharedMemory[u] =  si[u];
                }
            //}
        }

        vector<vector<int>> signatures = clustersIDs(sharedMemory, beta);

        cout << "Got Pure Signatures" << endl;
        // #pragma omp parallel for
        for (int u = 0; u < static_cast<int>(G.size()); ++u) {
            for (const vector<int>& signature : signatures) {
                if (similarity(si[u], signature) >= beta) {
                    // #pragma omp critical
                    C[u].push_back(signature);
                }
            }
        }

        elapsedTime = time(nullptr) - startTime; // used in printClustersToFile
    }

    const vector<vector<vector<int>>>& getResults() const {
        return C;
    }

    void printResults() const {
        for (int i = 0; i < static_cast<int>(C.size()); i++) {
            cout << "Clusters node " << i << " is in: " << endl;
            for (const auto & j : C[i]) {
                for (const int x : j) {
                    cout << x << " ";
                }
                cout << " / ";
            }
            cout << endl;
        }
    }

    void printHistoryToFile(const string& filename) {
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

    unordered_map<vector<int>, unordered_set<int>, vectorHash> getClusters() const {
        unordered_map<vector<int>, unordered_set<int>, vectorHash> clusters;
        for (int i = 0; i < static_cast<int>(C.size()); i++) {
            for (int j = 0; j < static_cast<int>(C[i].size()); j++) {
                clusters[C[i][j]].insert(i);
            }
        }
        return clusters;
    }

    void printClusters() const {
        const auto clusters = getClusters();
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

    void printClustersToFile(const string& name) const {
        ofstream f;
        f.open(name);
        int i = 0;
        const auto clusters = getClusters();
        // Cannot use structured bindings here with c++11
        // ReSharper disable once CppUseStructuredBinding
        for (const auto& cluster : clusters) {
            f << "Cluster " << ++i << ": ";
            for (const int num : cluster.first) {
                f << num << " ";
            }
            f << endl << "Nodes: ";
            for (const int num : cluster.second) {
                f << num << " ";
            }
            f << endl;
        }
        f << "Time taken: ~" << elapsedTime <<  "s" << endl;
        f.close();
    }

};



#endif // OVERCODE_H_INCLUDED
