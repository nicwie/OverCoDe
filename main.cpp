#include <iostream>
#include <cstdlib>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <random>
#include <string>
#include "include/OverCoDe.h"
#include "include/ClusteredGraph.h"
#include "include/SyntheticEgoGraph.h"
#include "include/Graph.h"

//#include "DistributedProcess.h"






using namespace std;

    bool contains (const vector<int>& g, const int n) {
        return any_of(g.begin(), g.end(), [&n](const int i) { return i == n; });
    }

    vector<vector<int>> readGraphFromFile(const string& filename) {


        vector<vector<int>> adjList;
        ifstream a;
        a.open(filename);
        string read;
        while (a) {
            a >> read;
            int node1 = stoi(read);
            a >> read;
            int node2 = stoi(read);
            if (node1 >= static_cast<int>(adjList.size())) {
                adjList.resize(node1 + 1);
            }
            if (node2 >= static_cast<int>(adjList.size())) {
                adjList.resize(node2 + 1);
            }

            if (contains(adjList[node1], node2) || contains(adjList[node2], node1)) {
                continue;
            }
            adjList[node1].push_back(node2);
            adjList[node2].push_back(node1);
        }
        return adjList;
    }


int main(int argc, char * argv[]) {

    auto startTime = time(nullptr);

    int n = 0;
    // int n = 5000;
    // int n = 125; // For ego graph
    // ReSharper disable once CppTooWideScope
    double beta;
    // beta = 0.95; // similarity threshold
    // double beta = 0.85; // For ego graph
    // ReSharper disable once CppTooWideScope
    double alpha;
    // alpha = 0.92; // Majority (history) threshold (0.9 | 0.92)

    int T = 0;
    // int T = static_cast<int>(10 * log2(n));   // Number of rounds
    // int T = static_cast<int>(50 * log2(n)); // For ego graph
    int k = 0;   // Number of pushes
    // ReSharper disable once CppTooWideScope
    int rho = 3; // Number of majority samples
    int l = 0;
    // int l = T; // Number of iterations
    // int l = static_cast<int>(250 * log2(n)); // For ego graph

    bool isEgoGraph = false;
    int graphs = 0, runs = 0;
    string filename;
    vector<unsigned long long> overlaps{0};


    try {
        double p = 0;
        if (argc < 7) {
            cerr << "Not enough arguments! Usage: ./main <Is ego Graph?: true | false> alpha beta OutputFile Graphs Runs overlapSize [overlapSize [overlapSize ...]]" << endl;
            return -1;
        }

        //if (stod(argv[3]) > 1 || stod(argv[3]) <= 0 || stod(argv[4]) > 1 || stod(argv[4]) <= 0) {
        //    cerr << "Alpha and beta must be between 1 and 0!" << endl;
        //    return -1;
        //}

        alpha = stod(argv[2]);
        beta = stod(argv[3]);

        if (stoi(argv[5]) < 1 || stoi(argv[6]) < 1) {
            cerr << "Graphs and Runs must be >= 1!" << endl;
            return -1;
        }

        filename = static_cast<string>(argv[4]);
        graphs = stoi(argv[5]);
        runs = stoi(argv[6]);

        if (static_cast<string>(argv[1]) == "true") {
            if (argc != 7) {
                cout << "Usage: ./main true alpha beta OutputFile Graphs Runs" << endl;
                return -1;
            }

            isEgoGraph = true;
            n = 125;
            T = static_cast<int>(50 * log2(n));
            l = static_cast<int>(250 * log2(n));
            p = pow(static_cast<double>(n), 0.75) / n;
            k = static_cast<int>(log2(n) / p);

        } else if (static_cast<string>(argv[1]) == "false"){
            if (argc <= 7) {
                cout << "Usage: ./main false OutputFile Graphs Runs overlapSize [overlapSize [overlapSize ...]]" << endl;
                return -1;
            }

            for (int i = 7; i < argc; i++) {
                overlaps.push_back(stoull(argv[i]));
            }
            isEgoGraph = false;
            n = 5000;
            T = static_cast<int>(10 * log2(n));
            l = T;
            p = pow(static_cast<double>(n), 0.75) / n;
            k = static_cast<int>(log2(n) / p);
        }

    } catch([[maybe_unused]] exception &e) {
        cout << "Oh no! " << e.what() << endl;
        return -1;
    }

    cout << "Running with " << graphs << " graphs and " << runs << " runs." << endl;
    if (!isEgoGraph) {
        bool first = true;
         cout << "Cluster Graph, with overlaps: " << endl;
        for (unsigned long long overlap : overlaps) {
            // this is done so that we do not print the overlap at overlaps[0], which is always 0
            // overlaps[0] is zero because we calculate the size of the cluster without
            // its overlaps in ClusteredGraph.h
            if (first) {
                first = false;
                continue;
            }
            cout << overlap << " ";
        }
        cout << endl;
    } else {
        cout << "Ego graph." << endl;
    }





    ofstream a;
    a.open(filename); // done to delete file contents if file already exists
    a.close();

    a.open(filename + "_truth");
    a.close();

    cout << "Before graph" << endl;

    unique_ptr<Graph> graph;

    if (isEgoGraph) {
        // graph = make_unique<SyntheticEgoGraph>(); // only cpp14
        // ReSharper disable once CppSmartPointerVsMakeFunction
        graph = unique_ptr<Graph>(new SyntheticEgoGraph());
    } else {
        // graph = make_unique<ClusteredGraph>(n, overlaps); // only cpp14
        // ReSharper disable once CppSmartPointerVsMakeFunction
        graph = unique_ptr<Graph>(new ClusteredGraph(n, overlaps));
    }

    for (int i = 0; i < graphs; i++) {
        // have "graph" as abstract class and implement as cluster & ego?

        graph->generateGraph();

        // vector<vector<int>> adjList = readGraphFromFile("email-Eu-core.txt");
        cout << "Graph created" << endl;



        for (int j = 0; j < runs; j++) {

            cout << "[" << i << "]" << "[" << j << "]" << endl;
            //graph->printGraph();
            auto* ocd = new OverCoDe(graph->getAdjList(), T, k, rho, l, beta, alpha);
            //OverCoDe* ocd = new OverCoDe(adjList, T, k, rho, l, alpha_1, alpha_2);
            ocd->runOverCoDe();


            ofstream f;
            f.open(filename, std::ofstream::app);
            int c = 0;

            f << i << " " << j << endl;
            a.open(filename + "_truth", std::ofstream::app);
            a << i << " " << j << endl;
            a.close();
            graph->appendTruthToFile(filename + "_truth");

            auto clusters = ocd->getClusters();
            // Cannot use structured bindings in cpp11
            // ReSharper disable once CppUseStructuredBinding
            for (const auto& cluster : clusters) {
                f << "Cluster " << ++c << ": ";
                for (int num : cluster.first) {
                    f << num << " ";
                }
                f << endl;
                for (int num : cluster.second) {
                    f << num << " ";
                }
                f << endl;
                f << endl;
            }
            f << endl << endl;
            f.close();
            // ocd->printClusters();
            cout << c << " clusters." << endl;
            //ocd->printHistoryToFile(filename + "_signatures");
            delete ocd;
        }
        graph->deleteGraph();
    }

    auto elapsedTime = time(nullptr) - startTime;

    // ReSharper disable once CppRedundantParentheses
    cout << ((elapsedTime / 60) / 60) << "h " << (elapsedTime / 60) % 60 << "min " << elapsedTime % 60 <<  "s" << endl;


    return 0;
}
