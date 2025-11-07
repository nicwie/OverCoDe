#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "OverCoDe.h"

TEST(OverCoDeTest, Semaphore) {
  Semaphore sem(1);

  sem.acquire(); // Acquire the only permit

  std::atomic<bool> threadAcquired(false);

  std::thread t1([&]() {
    sem.acquire(); // This should block
    threadAcquired = true;
    sem.release();
  });

  // Thread t1 should be blocked, so threadAcquired should be false
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(threadAcquired);

  sem.release(); // Release the permit

  // Thread t1 should now acquire it
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(threadAcquired);

  t1.join();
}

TEST(OverCoDeTest, RunSmokeTest) {
  // Create a simple graph: two 3-node cliques (0-1-2) and (2-3-4)
  // Node 2 is the overlap
  std::vector<std::vector<unsigned long long>> adjList(5);
  adjList[0] = {1, 2};
  adjList[1] = {0, 2};
  adjList[2] = {0, 1, 3, 4};
  adjList[3] = {2, 4};
  adjList[4] = {2, 3};

  // Parameters for OverCoDe
  int T = 10;
  int k = 2;
  int rho = 2;
  size_t ell = 20;
  double beta = 0.6;
  double alpha = 0.6;

  OverCoDe overcode(adjList, T, k, rho, ell, beta, alpha);

  // Just run the algorithm. We can't easily check the output due to randomness,
  // so this is a "smoke test" to see if it runs without crashing.
  ASSERT_NO_THROW(overcode.runOverCoDe());

  // Basic sanity check on the results
  const auto &results = overcode.getResults();
  EXPECT_EQ(results.size(), 5); // One result per node

  const auto clusters = overcode.getClusters();
  // It's possible (though unlikely) no clusters are found if beta/alpha are too
  // high or 'ell' is too low, but with this graph, we expect *some* result.

  // Example: Check that node 2 (the overlap) is in more or equal clusters
  // than node 0 (non-overlap). This might not always hold due to randomness,
  // but it's a reasonable heuristic.
  EXPECT_GE(results[2].size(), results[0].size());
}
