import sys
import time


def readClusterFile(filename):
	clusters = {}
	currentGraph = None
	currentClusters = []
	
	with open(filename, 'r') as file:
		for line in file:
			
			if len(line.strip().split()) == 2:
				parts = line.strip().split()
				graphNum, totalGraphs = map(int, parts)
				
				if currentGraph is not None:
					clusters[currentGraph] = currentClusters
				currentGraph = (graphNum, totalGraphs)
				currentClusters = []
				
			if len(line.strip().split()) > 300:
				# this only works when clusters are actually larger than 300 every time - it should be good for clusters of size 2500
				nodes = list(map(int, line.split()))
				currentClusters.append(nodes)
				
			else:
				continue

		if currentGraph is not None:
			clusters[currentGraph] = currentClusters

	return clusters
	
def calculateMisclassifications(clusters):
	timestamp = time.strftime("%Y%m%d_%H%M%S")
	filename = f"graph_errors_{timestamp}.txt"
	
	totalMisclassed = 0
	totalMisclassedOverlap = 0
	totalMissedNodes = 0
	totalCluster1 = 0
	totalCluster2 = 0
	notClustered = 0
	
	currentMissedCluster1 = 0
	currentMissedCluster2 = 0
	currentTotalCluster1 = 0
	currentTotalCluster2 = 0
	currentMisclassed = 0
	currentMisclassedOverlap = 0
	currentMissedNodes = 0
	i = 0
	
	with open(filename, 'w') as file:
        
            # file.write(" ".join(map(str, community)) + "\n")
		for graph, clusters in clusters.items():
			overlapMissed = 0
			totalMissedCluster1 = 0
			totalMissedCluster2 = 0
		
			if len(clusters) != 2:
				i+= 1
				notClustered += 1
				print()
				print(f"--------------------------------------")
				print(f"Graph [{graph[0]}][{graph[1]}] Does not have 2 clusters!")
				print(f"Has {len(clusters)}")
				print(f"--------------------------------------")
				print()
				file.write("\n")
				file.write(f"--------------------------------------\n")
				file.write(f"Graph [{graph[0]}][{graph[1]}] Does not have 2 clusters!\n")
				file.write(f"Has {len(clusters)}\n")
				file.write(f"--------------------------------------\n")
				file.write("\n")
				continue
			
			cluster1, cluster2 = clusters
		
			cluster2Range = set(range(0,2500))
			cluster1Range = set(range(2450, 4950))
			overlapRange = set(range(2450, 2500))
		
			for node in overlapRange:
				if node not in cluster1 or node not in cluster2:
					overlapMissed += 1
		
			for node in  cluster1Range:
				if node not in cluster1:
					totalMissedCluster1 += 1
			
			for node in cluster2Range:
				if node not in cluster2:
					totalMissedCluster2 += 1
		
		
			totalMisclassedOverlap += overlapMissed
			currentMissedNodes += overlapMissed
		
			"""
			print(f"Graph {graph[0]} : {graph[1]}:")
			print(f"  Cluster 1 misclassified: {totalMissedCluster1}/{len(cluster1)}")
			print(f"  Cluster 2 misclassified: {totalMissedCluster2}/{len(cluster2)}")
			print(f"  Overlap misclassified: {overlapMissed}/{len(overlapRange)}")
			"""
		
			currentMissedCluster1 += totalMissedCluster1
			currentMissedCluster2 += totalMissedCluster2
			totalMisclassed += totalMissedCluster1 + totalMissedCluster2
		
			currentTotalCluster1 += len(cluster1)
			currentTotalCluster2 += len(cluster2)
		
			totalCluster1 += len(cluster1)
			totalCluster2 += len(cluster2)
		
			i += 1
			# Check every 20 Cluster pairs (eg after 1 whole graph)
			if i == 20:
				
				print(f"Graph [{graph[0]}]:")
				print(f" Nodes misclassified: {currentMissedCluster1 + currentMissedCluster2} / {currentTotalCluster1 + currentTotalCluster2} = {((currentMissedCluster1 + currentMissedCluster2) / (currentTotalCluster1 + currentTotalCluster2)):.4f}")
				currentMisclassed = 0
				print(f" Nodes missed in first cluster: {currentMissedCluster1} / {currentTotalCluster1}")
				print(f" Nodes missed in second cluster: {currentMissedCluster2} / {currentTotalCluster2}")
				print(f" Missed nodes: {currentMissedNodes} / {50 * i} = {(currentMissedNodes / (50 * i)):.4f}")
				print()
				
				file.write(f"Graph [{graph[0]}]:\n")
				file.write(f" Nodes misclassified: {currentMissedCluster1 + currentMissedCluster2} / {currentTotalCluster1 + currentTotalCluster2} = {((currentMissedCluster1 + currentMissedCluster2) / (currentTotalCluster1 + currentTotalCluster2)):.4f}\n")
				file.write(f" Nodes missed in first cluster: {currentMissedCluster1} / {currentTotalCluster1}\n")
				file.write(f" Nodes missed in second cluster: {currentMissedCluster2} / {currentTotalCluster2}\n")
				file.write(f" Missed nodes: {currentMissedNodes} / {50 * i} = {(currentMissedNodes / (50 * i)):.4f}\n")
				file.write("\n")
				currentMissedNodes = 0
				currentMissedCluster1 = 0
				currentMissedCluster2 = 0
				currentTotalCluster1 = 0
				currentTotalCluster2 = 0
				i = 0
		
		#Global Misclassifications:
		print()
		print()
		print(f" Global misclassifications: {totalMisclassed} / {totalCluster1 + totalCluster2} = {(totalMisclassed / (totalCluster1 + totalCluster2)):.4f}")
		print(f" Gobal overlap misclassifications: {totalMisclassedOverlap} / {50 * 400} = {(totalMisclassedOverlap / (50 * 400)):.4f}")
		print(f" Graphs where clusters where not identified correctly: {notClustered} / 400")
		file.write("\n")
		file.write("\n")
		file.write(f" Global misclassifications: {totalMisclassed} / {totalCluster1 + totalCluster2} = {(totalMisclassed / (totalCluster1 + totalCluster2)):.4f}\n")
		file.write(f" Gobal overlap misclassifications: {totalMisclassedOverlap} / {50 * 400} = {(totalMisclassedOverlap / (50 * 400)):.4f}\n")
		file.write(f" Graphs where clusters where not identified correctly: {notClustered} / 400\n")
	



inputFile = sys.argv[1]

clusters = readClusterFile(inputFile)
calculateMisclassifications(clusters)

