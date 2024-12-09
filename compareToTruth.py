import argparse
import os
import time
from scipy.optimize import linear_sum_assignment
from collections import defaultdict
from statistics import mean, median, stdev
from itertools import chain

usage = """
usage: python3 compareToTruth <clusterFile> <truthFile> <graphs> <simulations per graph> [--name: output file note] [-m: minimal output]

"""


def read_cluster_file(filename):
    read_clusters = {}
    current_graph = None
    current_clusters = []

    with open(filename, 'r') as file:
        for line in file:

            if len(line.strip().split()) == 2:
                parts = line.strip().split()
                graph_num, total_graphs = map(int, parts)

                if current_graph is not None:
                    read_clusters[current_graph] = current_clusters

                current_graph = (graph_num, total_graphs)
                current_clusters = []

            if len(line.strip().split()) > 3:
                if line.strip().split()[0] == "Cluster":
                    continue
                # this is done so that we do not count the cluster name / signature line as nodes
                nodes = list(map(int, line.split()))
                current_clusters.append(nodes)

            else:
                continue

        if current_graph is not None:
            read_clusters[current_graph] = current_clusters

    return read_clusters


def read_truth(filename):
    read_clusters = {}
    current_graph = None
    current_clusters = []

    with open(filename, 'r') as file:
        for line in file:

            if len(line.strip().split()) == 2:
                if line.strip().split()[0] == "Cluster":
                    continue
                parts = line.strip().split()
                graph_num, total_graphs = map(int, parts)

                if current_graph is not None:
                    read_clusters[current_graph] = current_clusters

                current_graph = (graph_num, total_graphs)
                current_clusters = []

            if len(line.strip().split()) > 3:
                nodes = list(map(int, line.split()))
                current_clusters.append(nodes)

            else:
                continue

        if current_graph is not None:
            read_clusters[current_graph] = current_clusters

    return read_clusters


def display_and_save_results(results, jaccard_summary_data, correctly_clustered, output_file, minimal=False):
    """
    Print the results and save them to a file, updated to handle graph IDs.

    Args:
        :param correctly_clustered: The amount of graphs that were clustered correctly (all clusters were matched)
        :param jaccard_summary_data: A dictionary of overlaps and their average jaccard scores with standard deviations.
        :param results: List of dictionaries with 'graph_id', 'cluster_index', 'truth_index',
                 'extra_nodes', and 'missing_nodes' keys.
        :param output_file: Path to the file where the results will be saved.
        :param minimal: If True, display minimal output.
    """

    results.sort(key=lambda x: x['graph_id'])

    total_extra_nodes = 0
    total_missing_nodes = 0
    total_truth_size = 0

    with open(output_file, "w") as file:
        header = "Cluster Comparison Results by Graph\n" if not minimal else "Minimal Results\n"
        print(header)
        file.write(header)

        for result in results:
            graph_id = result["graph_id"]
            cluster_idx = result["cluster_index"]
            truth_idx = result["truth_index"]
            extra_nodes = result["extra_nodes"]
            missing_nodes = result["missing_nodes"]
            truth_size = result["truth_size"]

            # Update totals
            total_extra_nodes += len(extra_nodes)
            total_missing_nodes += len(missing_nodes)
            total_truth_size += len(truth_size)
            if not minimal:
                comparison_result = (
                    f"Graph ID: {graph_id}\n"
                    f"Cluster Index: {cluster_idx if cluster_idx is not None else 'Unmatched'}\n"
                    f"Truth Index: {truth_idx if truth_idx is not None else 'Unmatched'}\n"
                    f"Extra Nodes: {extra_nodes}\n"
                    f"Missing Nodes: {missing_nodes}\n"
                    f"Size: {len(truth_size)}\n"
                    "----------------------------------------\n"
                )
                print(comparison_result)
                file.write(comparison_result)
        # Sum results
        summary = (
            f"\nSummary of Results:\n"
            f"Total Extra Nodes: {total_extra_nodes}\n"
            f"Total Missing Nodes: {total_missing_nodes}\n"
            f"Total Truth Size: {total_truth_size}\n"
        )

        num_graphs = len(results)
        avg_extra_nodes = total_extra_nodes / num_graphs if num_graphs > 0 else 0
        avg_missing_nodes = total_missing_nodes / num_graphs if num_graphs > 0 else 0
        avg_truth_size = total_truth_size / num_graphs if num_graphs > 0 else 0

        summary += (
            f"Average Extra Nodes per Graph: {avg_extra_nodes:.2f}\n"
            f"Average Missing Nodes per Graph: {avg_missing_nodes:.2f}\n"
            f"Average Truth Size per Graph: {avg_truth_size:.2f}\n"
            f"Correctly Clustered Graphs: {correctly_clustered}\n"
        )

        # Iterate through each cluster count and corresponding statistics
        jacc_sum = "\nJaccard Index Statistics by Total Ground Truth Clusters and Node Cluster Membership:\n"
        jacc_sum += "-" * 80 + "\n"
        jacc_sum += f"{'Truth Clusters':<15} {'Node Clusters':<15} {'Mean':<10} {'Std Dev':<10}\n"
        jacc_sum += "-" * 80 + "\n"

        for total_truth_clusters, cluster_data in sorted(jaccard_summary_data.items()):
            for cluster_count, stats in sorted(cluster_data.items()):
                jacc_sum += (
                    f"{total_truth_clusters:<15} {cluster_count:<15} "
                    f"{stats['mean']:<10.4f} {stats['std_dev']:<10.4f}\n"
                )

        jacc_sum += "-" * 80 + "\n"
        """
        jacc_sum = "\nJaccard Index Statistics by Number of Clusters:\n"
        jacc_sum += "Clusters Count | Mean Jaccard | Std Dev\n"
        jacc_sum += "-" * 60 + "\n"

        # Iterate through each cluster count and corresponding statistics
        for cluster_count, stats in sorted(jaccard_summary_data.items()):
            mean = stats["mean"]
            std_dev = stats["std_dev"]
            jacc_sum += f"{cluster_count:>14} | {mean:>13.4f} | {std_dev:>8.4f}\n"
        """
        summary += jacc_sum

        print(summary)
        file.write(summary)

    print(f"\nResults have been saved to {output_file}\n")


def transform_clusters(clustering_results, ground_truth_clusters):
    """
    Transforms clustering results and ground truth clusters into a list of dictionaries
    suitable for compute_jaccard_and_average.

    Args:
        :param clustering_results: Dictionary of graph IDs to arrays of clusters (clustering result).
        :param ground_truth_clusters: Dictionary of graph IDs to arrays of clusters (ground truth).

    Returns:
        A list of dictionaries with 'node_clusters' and 'ground_truth_clusters' for each graph.
    """
    transformed_results = []


    all_graphs = set(clustering_results.keys()).union(set(ground_truth_clusters.keys()))

    for graph_id in all_graphs:

        node_clusters = defaultdict(set)
        node_truth_clusters = defaultdict(set)

        # Get clusters for this graph, defaulting to empty lists if missing
        result_clusters = clustering_results.get(graph_id, [])
        truth_clusters = ground_truth_clusters.get(graph_id, [])

        # Process clustering result
        for cluster_idx, cluster in enumerate(result_clusters):
            for node in cluster:
                node_clusters[node].add(cluster_idx)

        # Process ground truth clusters
        for cluster_idx, cluster in enumerate(truth_clusters):
            for node in cluster:
                node_truth_clusters[node].add(cluster_idx)

        # Combine into a result dictionary
        transformed_results.append({
            "node_clusters": dict(node_clusters),
            "ground_truth_clusters": dict(node_truth_clusters),
        })

    return transformed_results

def match_clusters_by_jaccard(cluster_graph, truth_graph):
    """
    Matches clusters to ground truth clusters based on Jaccard similarity.
    Ensures all clusters (and ground truths) are included, with unmatched ones mapped to None.

    Args:
        cluster_graph: List of clusters (each a list of integer nodes).
        truth_graph: List of ground truth clusters (each a list of integer nodes).

    Returns:
        A list of tuples representing the matched (cluster_index, truth_index).
        Includes unmatched clusters with None as the index for the missing pair.
    """
    cluster_sets = [set(cluster) for cluster in cluster_graph]
    truth_sets = [set(truth) for truth in truth_graph]
    if not cluster_sets or not truth_sets:
        return []
    # Compute the Jaccard similarity matrix
    overlap_matrix = [
        [jaccard_similarity(c, t) for t in truth_sets] for c in cluster_sets
    ]

    # Perform Hungarian algorithm (maximize similarity by minimizing -similarity)
    matches = match_clusters(overlap_matrix)

    # Add unmatched clusters and truths
    unmatched_clusters = set(range(len(cluster_sets))) - {r for r, _ in matches}
    unmatched_truths = set(range(len(truth_sets))) - {c for _, c in matches}

    # Append unmatched clusters as (cluster_idx, None)
    matches.extend([(uc, None) for uc in unmatched_clusters])

    # Append unmatched truths as (None, truth_idx)
    matches.extend([(None, ut) for ut in unmatched_truths])

    return matches

def calculate_metrics(cluster_data, truth_data):
    """
    Analyze node differences between clusters and the ground truth for each graph.

    Args:
        cluster_data: Dictionary where keys are graph IDs (e.g., (graph_num, total_graphs))
                      and values are lists of clusters (each a list of integer nodes).
        truth_data: Same format as cluster_data, representing the ground truth.

    Returns:
        results: A list of dictionaries for each cluster comparison across all graphs.
    """
    results = []
    correctly_clustered = 0

    # Iterate over all graphs present in either clusters or truth data
    all_graphs = set(cluster_data.keys()).union(set(truth_data.keys()))

    for graph_id in all_graphs:
        cluster_graph = cluster_data.get(graph_id, [])
        truth_graph = truth_data.get(graph_id, [])

        cluster_sets = [set(cluster) for cluster in cluster_graph]
        truth_sets = [set(cluster) for cluster in truth_graph]
        if len(cluster_sets) == len(truth_sets):
            correctly_clustered += 1
        if not cluster_sets:
            continue
        # Create pairwise overlap matrix (Jacquard similarity) for all cluster-truth pairs
        overlap_matrix = [[jaccard_similarity(c, t) for t in truth_sets] for c in cluster_sets]

        # Match clusters within this graph
        cluster_to_truth = match_clusters(overlap_matrix)

        # Compare matched clusters
        for cluster_idx, truth_idx in cluster_to_truth:
            cluster = cluster_sets[cluster_idx]
            truth_cluster = truth_sets[truth_idx]

            extra_nodes = cluster - truth_cluster
            missing_nodes = truth_cluster - cluster

            results.append({
                "graph_id": graph_id,
                "cluster_index": cluster_idx,
                "truth_index": truth_idx,
                "extra_nodes": sorted(extra_nodes),
                "missing_nodes": sorted(missing_nodes),
                "truth_size": sorted(truth_cluster),
            })

        # Handle unmatched clusters (if any)
        unmatched_clusters = set(range(len(cluster_sets))) - {c[0] for c in cluster_to_truth}
        unmatched_truths = set(range(len(truth_sets))) - {c[1] for c in cluster_to_truth}

        for cluster_idx in unmatched_clusters:
            results.append({
                "graph_id": graph_id,
                "cluster_index": cluster_idx,
                "truth_index": None,
                "extra_nodes": sorted(cluster_sets[cluster_idx]),
                "missing_nodes": [],
                "truth_size": sorted(truth_sets[truth_idx]),
            })

        for truth_idx in unmatched_truths:
            results.append({
                "graph_id": graph_id,
                "cluster_index": None,
                "truth_index": truth_idx,
                "extra_nodes": [],
                "missing_nodes": sorted(truth_sets[truth_idx]),
                "truth_size": sorted(truth_sets[truth_idx]),
            })

    return results, correctly_clustered


def jaccard_similarity(set1, set2):
    """
    Calculate Jaccard Similarity between two sets.
    """
    intersection = len(set1 & set2)
    union = len(set1 | set2)
    return intersection / union if union > 0 else 0


def match_clusters(overlap_matrix):
    """
    Match clusters based on highest overlap scores (Jaccard Similarity).
    
    Args:
        overlap_matrix: 2D list where overlap_matrix[i][j] is the Jacquard similarity
                        between cluster i and truth cluster j.

    Returns:
        A list of matched cluster pairs as (cluster_idx, truth_idx).
    """

    # Convert to a cost matrix (maximize similarity → minimize negative similarity)
    cost_matrix = [[-value for value in row] for row in overlap_matrix]

    # Solve assignment problem
    cluster_indices, truth_indices = linear_sum_assignment(cost_matrix)

    return list(zip(cluster_indices, truth_indices))

def compute_jaccard_and_average(results):
    """
    Compute the Jaccard index for nodes based on their cluster memberships
    and group the averages by the number of clusters a node belongs to.
    Args:
        results: List of dictionaries, where each dictionary contains:
                 - 'node_clusters': A dictionary mapping nodes to sets of cluster indices (from clustering result).
                 - 'ground_truth_clusters': A dictionary mapping nodes to sets of ground truth cluster indices.
    Returns:
        A dictionary mapping the number of clusters a node belongs to
        to a dictionary with statistics (mean, standard deviation, median) for Jaccard indices.
    """
    # To store Jaccard indices by ground truth cluster count &  cluster count
    grouped_jaccard_indices = defaultdict(lambda: defaultdict(list))
    # node_jaccard_indices = defaultdict(list)
    for result in results:

        node_clusters = result["node_clusters"]
        ground_truth_clusters = result["ground_truth_clusters"]
        # flatten sets of indices into one iterable and count unique
        total_truth_clusters = len(set(chain.from_iterable(ground_truth_clusters.values())))

        all_nodes = set(node_clusters.keys()).union(ground_truth_clusters.keys())

        for node in all_nodes:
            clusters = node_clusters.get(node, set())
            ground_truth = ground_truth_clusters.get(node, set())

            # Compute Jaccard index
            jaccard_index = jaccard_similarity(clusters, ground_truth)

            # Group by the number of clusters the node belongs to
            cluster_count = len(ground_truth)
            grouped_jaccard_indices[total_truth_clusters][cluster_count].append(jaccard_index)

    # compute mean, std dev, and median for each group
    statistics_by_group = {}
    for total_truth_clusters, cluster_count_data in grouped_jaccard_indices.items():
        statistics_by_group[total_truth_clusters] = {}
        for cluster_count, indices in cluster_count_data.items():
            statistics_by_group[total_truth_clusters][cluster_count] = {
                "mean": mean(indices),
                "std_dev": stdev(indices) if len(indices) > 1 else 0.0,  # stdev requires at least 2 values
            }
        """
        all_nodes = set(node_clusters.keys()).union(ground_truth_clusters.keys())

        for node in all_nodes:
            # Convert lists to sets for Jaccard computation
            clusters = set(node_clusters.get(node, []))
            ground_truth = set(ground_truth_clusters.get(node, []))

            # Compute Jaccard index
            jaccard_index = jaccard_similarity(clusters, ground_truth)

            # Group by the number of clusters the node belongs to
            cluster_count = len(ground_truth)
            node_jaccard_indices[cluster_count].append(jaccard_index)

    # Compute statistics (mean, std dev, and median) for each group


    statistics_by_group = {}
    for cluster_count, indices in node_jaccard_indices.items():
        stats = {
            "mean": mean(indices),
            "std_dev": stdev(indices) if len(indices) > 1 else 0.0,  # stdev requires at least 2 values
        }
        statistics_by_group[cluster_count] = stats
    """
    return statistics_by_group

def match_clusters_jaccard(clusters, truth):
    matched_clusters = {}
    matched_truth = {}

    for graph_id in clusters.keys():

        cluster_graph = clusters.get(graph_id, [])
        truth_graph = truth.get(graph_id, [])

        indices = match_clusters_by_jaccard(cluster_graph, truth_graph)

        matched_clusters[graph_id] = []
        matched_truth[graph_id] = []

        for cluster_idx, truth_idx in indices:

            if cluster_idx is not None and truth_idx is not None:
                matched_clusters[graph_id].append(cluster_graph[cluster_idx])
                matched_truth[graph_id].append(truth_graph[truth_idx])

            elif cluster_idx is not None:
                matched_clusters[graph_id].append(cluster_graph[cluster_idx])
                matched_truth[graph_id].append([])

            elif truth_idx is not None:
                matched_clusters[graph_id].append([])
                matched_truth[graph_id].append(truth_graph[truth_idx])

    return matched_clusters, matched_truth

def compare_files(clusters_file, truth_file, graphs, runs, name, minimal):
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    filename = f"errors/graph_errors_{timestamp}_{graphs}x{runs}{name}.txt"

    os.makedirs(os.path.dirname(filename), exist_ok=True)

    results, correctly_clustered = calculate_metrics(clusters_file, truth_file)

    matched_clusters, matched_truth = match_clusters_jaccard(clusters_file, truth_file)
    matched_node_list = transform_clusters(matched_clusters, matched_truth)
    avg_jaccard = compute_jaccard_and_average(matched_node_list)
    display_and_save_results(results, avg_jaccard, correctly_clustered, filename, minimal)



def main():
    parser = argparse.ArgumentParser(description="Analyze clusters compared to ground truth")
    parser.add_argument('inputFile', type=str, help="Cluster file")
    parser.add_argument('truthFile', type=str, help="Ground truth file")
    parser.add_argument('graphs', type=int, help="Number of graphs")
    parser.add_argument('runs', type=int, help="Simulations per graph")
    parser.add_argument('--name', type=str, help="Output file note", default="")
    parser.add_argument('-m', action='store_true', help="Minimal output")
    args = parser.parse_args()
    clusters = read_cluster_file(args.inputFile)
    truth = read_truth(args.truthFile)
    compare_files(clusters, truth, args.graphs, args.runs, args.name, args.m)

if __name__ == "__main__":
    main()