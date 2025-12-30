#ifndef OVERCODE_H_INCLUDED
#define OVERCODE_H_INCLUDED

#include "RandomGenerator.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <ctime>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Define the types of tokens
enum Token { R, B };

namespace std {
template <> struct hash<Token> {
  size_t operator()(const Token &token) const noexcept {
    return static_cast<size_t>(token);
  }
};
} // namespace std

class Semaphore {
private:
  std::mutex mtx;
  std::condition_variable cv;
  int count;

public:
  explicit Semaphore(const int maxThreads) : mtx(), count(maxThreads) {}

  void acquire() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() { return count > 0; });
    count--;
  }

  void release() {
    std::unique_lock<std::mutex> lock(mtx);
    count++;
    cv.notify_one();
  }
};

class OverCoDe {
private:
  std::vector<std::vector<unsigned long long>> G;
  int T, k, rho, h;
  size_t ell;
  double beta, alpha;

  time_t startTime{}, elapsedTime{};
  std::vector<std::vector<int>> si;
  std::vector<std::vector<std::vector<int>>> C;

  static constexpr int maxThreads = 16;

  struct vectorHash {
    size_t operator()(const std::vector<int> &v) const {
      size_t seed = v.size();
      for (auto &i : v) {
        seed ^= static_cast<size_t>(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };

  // Function to randomly initialize the tokens
  static Token randomToken() {
    return static_cast<Token>(rng.getRandomInt(0, 1));
  }

  // Function to execute the distributed process
  std::vector<Token>
  distributedProcess(const std::vector<std::vector<unsigned long long>> &graph,
                     size_t T_dist, const int k_dist, const int rho_dist,
                     const int h_dist,
                     std::vector<int> &receivedR, // Reusable scratch buffer
                     std::vector<int> &receivedB  // Reusable scratch buffer
  ) const {
    size_t n = graph.size();
    size_t rounds = T_dist + 2;

    // Flattened X: row-major [u * rounds + t]
    std::vector<Token> X(n * rounds);

    // Reset scratch buffers
    std::fill(receivedR.begin(), receivedR.end(), 0);
    std::fill(receivedB.begin(), receivedB.end(), 0);

    // Random Initialization
    for (size_t u = 0; u < n; u++) {
      X[u * rounds + 0] = randomToken();
    }

    // Symmetry Breaking
    // Step 1: Push tokens to k neighbors
    for (size_t u = 0; u < n; u++) {
      const auto &neighbors = graph[u];
      if (neighbors.empty())
        continue;

      Token val = X[u * rounds + 0];
      size_t sz = neighbors.size();

      // Inline sampling
      for (int i = 0; i < k_dist; ++i) {
        // Optimization: Use fast RNG
        int idx = rng.getFastRandomInt(static_cast<int>(sz) - 1);
        unsigned long long v = neighbors[static_cast<size_t>(idx)];
        if (val == R) {
          ++receivedR[v];
        } else {
          ++receivedB[v];
        }
      }
    }

    // Step 2: Sample h neighbors and check their inboxes
    for (size_t u = 0; u < n; u++) {
      int r_u = 0;
      int b_u = 0;
      const auto &neighbors = graph[u];

      if (!neighbors.empty()) {
        size_t sz = neighbors.size();
        for (int i = 0; i < h_dist; i++) {
          // Optimization: Use fast RNG
          int idx = rng.getFastRandomInt(static_cast<int>(sz) - 1);
          unsigned long long v = neighbors[static_cast<size_t>(idx)];
          r_u += receivedR[v];
          b_u += receivedB[v];
        }
      }

      // Determine state based on sums
      X[u * rounds + 1] = ((r_u > b_u) ? R : (b_u > r_u) ? B : randomToken());
    }

    // ρ-Majority process
    for (size_t t = 2; t <= T_dist + 1; t++) {
      for (size_t u = 0; u < n; u++) {
        const auto &neighbors = G[u];
        int countB = 0;
        int countR = 0;

        if (!neighbors.empty()) {
          size_t sz = neighbors.size();
          for (int i = 0; i < rho_dist; i++) {
            // Optimization: Use fast RNG
            int idx = rng.getFastRandomInt(static_cast<int>(sz) - 1);
            unsigned long long v = neighbors[static_cast<size_t>(idx)];
            Token prev = X[v * rounds + (t - 1)];
            (prev == R) ? countR++ : countB++;
          }
        }

        X[u * rounds + t] = ((countR > countB)   ? R
                             : (countR < countB) ? B
                                                 : randomToken());
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
  static double similarity(const std::vector<int> &a,
                           const std::vector<int> &b) {
    int common = 0, validIndices = 0;

    for (size_t i = 0; i < std::min(a.size(), b.size()); i++) {
      if (a[i] != -1 && b[i] != -1) { // Ensure both indices are valid
        validIndices++;
        if (a[i] == b[i]) {
          common++;
        }
      }
    }

    if (validIndices == 0) {
      return 0.0; // Avoid division by zero if there are no valid indices
    }

    return static_cast<double>(common) /
           validIndices; // Ratio of matching valid states
  }

  static std::vector<std::vector<int>>
  clustersIDs(std::vector<std::vector<int>> &S,
              const double similarity_threshold) {
    std::vector<std::vector<int>> signatures;
    for (const auto &signatureV : S) {
      bool isUnique = true;
      for (const auto &signatureU : signatures) {
        if (similarity(signatureU, signatureV) >= similarity_threshold) {
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
  OverCoDe(const std::vector<std::vector<unsigned long long>> &adjList,
           const int rounds, const int pushes, const int majoritySamples,
           const int sampleSize, size_t L, const double p_beta,
           const double p_alpha)
      : G(adjList), T(rounds), k(pushes), rho(majoritySamples), h(sampleSize),
        ell(L), beta(p_beta), alpha(p_alpha) {
    si.resize(G.size());
    C.resize(G.size());
  }

  void runOverCoDe() {
    startTime = time(nullptr);

    // Generate Signatures
    // this provides a vector which has the most common result for every node in
    // each iteration
    si.resize(G.size());

    // Stores results from threads
    std::vector<std::vector<Token>> X(ell);

    // Atomic counter for distributing work
    std::atomic<size_t> nextTaskIndex{0};
    std::atomic<size_t> completedTasks{0};

    // Determine number of worker threads
    size_t numThreads = std::min(static_cast<size_t>(maxThreads), ell);
    std::vector<std::thread> workers;

    // Worker lambda
    auto worker = [this, &X, &nextTaskIndex, &completedTasks]() {
      // Thread-local scratch buffers to avoid reallocation
      std::vector<int> localRecR(this->G.size(), 0);
      std::vector<int> localRecB(this->G.size(), 0);

      while (true) {
        size_t i = nextTaskIndex.fetch_add(1);
        if (i >= this->ell) {
          return;
        }

        X[i] =
            distributedProcess(this->G, static_cast<size_t>(this->T), this->k,
                               this->rho, this->h, localRecR, localRecB);

        // size_t done = ++completedTasks;
        // if (done % 20 == 0 || done == this->ell) {
        //   std::cout << "Signatures progress: " << done << "/" << this->ell
        //             << std::endl;
        // }
      }
    };

    // Spawn workers
    for (size_t i = 0; i < numThreads; ++i) {
      workers.emplace_back(worker);
    }

    // Join workers
    for (auto &t : workers) {
      t.join();
    }

    // Count if a state appears more often than alpha * T
    size_t rounds = static_cast<size_t>(this->T + 2);

    for (size_t i = 0; i < ell; i++) {
      for (size_t u = 0; u < G.size(); ++u) {
        int countR = 0, countB = 0;
        // Count only over the majority dynamics rounds (indices 2 to T + 1)
        for (size_t hist_idx = 2; hist_idx <= static_cast<size_t>(this->T + 1);
             hist_idx++) {
          // Access flattened array
          Token t = X[i][u * rounds + hist_idx];
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

    std::cout << "Done generating signatures" << std::endl;

    std::vector<std::vector<int>> pureSignatures;

    // Identify Clusters
    for (size_t u = 0; u < G.size(); ++u) {
      // if (bernoulli(log((double)G.size()) / (double)G.size())) {
      if (static_cast<double>(count_if(si[u].begin(), si[u].end(),
                                       [](const int x) { return x != -1; })) >=
          beta * static_cast<double>(ell)) {
        pureSignatures.push_back(si[u]);
      }
      //}
    }

    std::vector<std::vector<int>> signatures =
        clustersIDs(pureSignatures, beta);

    std::cout << "Got Pure Signatures" << std::endl;
    // #pragma omp parallel for
    for (size_t u = 0; u < G.size(); ++u) {
      for (const std::vector<int> &signature : signatures) {
        if (similarity(si[u], signature) >= beta) {
          // #pragma omp critical
          C[u].push_back(signature);
        }
      }
    }

    elapsedTime = time(nullptr) - startTime; // used in printClustersToFile
  }

  const std::vector<std::vector<std::vector<int>>> &getResults() const {
    return C;
  }

  void printResults() const {
    for (size_t i = 0; i < C.size(); i++) {
      std::cout << "Clusters node " << i << " is in: " << std::endl;
      for (const auto &j : C[i]) {
        for (const int x : j) {
          std::cout << x << " ";
        }
        std::cout << " / ";
      }
      std::cout << std::endl;
    }
  }

  void printHistoryToFile(const std::string &filename) {
    std::ofstream c;
    c.open(filename);
    int i = 0;

    for (std::vector<int> &vec : si) {
      c << i << std::endl;
      i++;
      for (int &node : vec) {
        c << node << " ";
      }
      c << std::endl << std::endl;
    }

    c.close();
    std::cout << "history written" << std::endl;
  }

  std::unordered_map<std::vector<int>, std::unordered_set<int>, vectorHash>
  getClusters() const {
    std::unordered_map<std::vector<int>, std::unordered_set<int>, vectorHash>
        clusters;
    for (size_t i = 0; i < C.size(); i++) {
      for (size_t j = 0; j < C[i].size(); j++) {
        clusters[C[i][j]].insert(static_cast<int>(i));
      }
    }
    return clusters;
  }

  void printClusters() const {
    const auto clusters = getClusters();
    int i = 0;
    for (const auto &pair : clusters) {
      std::cout << "Cluster " << ++i << ": ";
      for (int num : pair.first) {
        std::cout << num << " ";
      }
      std::cout << std::endl << "Nodes: ";
      for (int num : pair.second) {
        std::cout << num << " ";
      }
      std::cout << std::endl;
    }
  }

  void printClustersToFile(const std::string &name) const {
    std::ofstream f;
    f.open(name);
    int i = 0;
    const auto clusters = getClusters();
    // Cannot use structured bindings here with c++11
    // ReSharper disable once CppUseStructuredBinding
    for (const auto &cluster : clusters) {
      f << "Cluster " << ++i << ": ";
      for (const int num : cluster.first) {
        f << num << " ";
      }
      f << std::endl << "Nodes: ";
      for (const int num : cluster.second) {
        f << num << " ";
      }
      f << std::endl;
    }
    f << "Time taken: ~" << elapsedTime << "s" << std::endl;
    f.close();
  }
};

#endif // OVERCODE_H_INCLUDED
