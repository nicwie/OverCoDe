#ifndef DISTRIBUTEDPROCESS_H_INCLUDED
#define DISTRIBUTEDPROCESS_H_INCLUDED


#include <vector>
#include <unordered_map>
#include <stdlib.h>
#include <algorithm>
#include <numeric>
#include <omp.h>
#include "RandomGenerator.h"

using namespace std;

// Define the types of tokens
enum Token { R, B };

class DistributedProcess {
private:
	vector<vector<int>> G; // Graph adjacency list
    int T; // Number of rounds
    int k; // Number of pushes
    int rho; // Number of majority samples

    int n; // Number of nodes in the graph


    vector<vector<Token>> X; // History configuration matrix
    vector<unordered_map<Token, int>> receivedToken;

	RandomGenerator rng;

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

public:
    DistributedProcess(vector<vector<int>>& graph, int rounds, int pushes, int majoritySamples)
        : G(graph), T(rounds), k(pushes), rho(majoritySamples), n(graph.size()), X(n, vector<Token> (T+1)), rng() {
    }

    // Function to execute the distributed process
    void runProcess() {
        receivedToken.resize(n);
        // Random Initialization
        for (int u = 0; u < n; u++) {
            X[u][0] = (randomToken());
        }

        // Symmetry Breaking
        #pragma omp parallel for
        for (int u = 0; u < n; u++) {
            vector<int> M = sample(k, G[u]);
            for (int v : M) {
                #pragma omp critical
                ++receivedToken[v][X[u][0]];
            }
        }

        #pragma omp parallel for
        for (int u = 0; u < n; u++) {
            // Save the most common state to matrix; if equal, randomize state
            #pragma omp critical
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
    }

    // Function to get the history of the state matrix
    const vector<vector<Token>>& getResult() const {
        return X;
    }

    // function to print the history of the state matrix
    void printStates() {
        for (int u = 0; u < (int) X.size(); u++) {
        cout << "Node " << u << " had states: ";
            for (int c : X[u]) {
                cout << c << " ";
            }
        cout << endl;
        }
    }

    // Prints debug info
    void printResults() {
        int countR = 0, countB = 0;
        cout << "Starting states: ";
        for (int i = 0; i < (int) X.size(); i++) {
            (X[i][0] == 0) ? countR++ : countB++;
        }
        cout << "R: " << countR << "/ B: " << countB << endl;

        cout << "After Symmetry Breaking: ";
        countR = 0;
        countB = 0;
        for (int i = 0; i < (int) X.size(); i++) {
            (X[i][1] == 0) ? countR++ : countB++;
        }
        cout << "R: " << countR << "/ B: " << countB << endl;

        cout << "End result: ";
        countR = 0;
        countB = 0;
        for (int i = 0; i < (int) X.size(); i++) {
            (X[i].back() == 0) ? countR++ : countB++;
        }
        cout << "R: " << countR << "/ B: " << countB << endl;

    }

};

#endif // DISTRIBUTEDPROCESS_H_INCLUDED
