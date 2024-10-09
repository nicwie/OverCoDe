import sys
import time
from math import comb

usage = """
usage: python3 count_errors.py <filename> <graphs> <simulations per graph> <expected cluster size> <expected n° of clusters> (<output file note>)

"""


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
				
			if len(line.strip().split()) > 3:
				if line.strip().split()[0] == "Cluster":
					continue
				# this is done so that we do not count the cluster name / signature line as nodes
				nodes = list(map(int, line.split()))
				currentClusters.append(nodes)
				
			else:
				continue

		if currentGraph is not None:
			clusters[currentGraph] = currentClusters

	return clusters


def calculateMisclassifications(clusters, graphs, runs, clusterSize, nrClusters, outputFile):
	timestamp = time.strftime("%Y%m%d_%H%M%S")
	filename = f"graph_errors_{timestamp}_{outputFile}.txt"
	
	# global values
	
	notClustered = 0
	overlap = [[0] * nrClusters for i in range(nrClusters)]
	
	overlapRatio = [0] * (nrClusters + 1)
	overlapRatio[2] = 30
	overlapRatio[3] = 10
	# overlapRatio[4] = 1
	
	totalOverlap = [0] * (nrClusters + 1)
	
	overlapCounts = [0] * (nrClusters  + 1)
		
	nExclusive = clusterSize
	
	unusedNodes = []
	
	for k in range(2, nrClusters + 1):
		nExclusive -= overlapRatio[k] * comb(nrClusters - 1, k - 1) #nodes that only appear in 1 graph
	
	overlapRatio[1] = nExclusive
	
	uniqueNodes = nrClusters * nExclusive
	
	for k in range(2, nrClusters + 1):
		uniqueNodes += overlapRatio[k] * comb(nrClusters, k)
	
	uniqueRange = list(range(0, uniqueNodes))
	
	# run values
	
	i = 0
	currentNotClustered = 0
	currentOverlap = [[0] * nrClusters for i in range(nrClusters)]
	currentOverlapSets = [[0] * nrClusters for i in range(nrClusters)]
	
	
	for k in range(nrClusters):
		for j in range(k + 1, nrClusters):
			currentOverlap[k][j] = 0
			overlap[k][j] = 0
	
	
	with open(filename, 'w') as file:
		for graph, clusters in clusters.items():
			
			
			i += 1
			
			
			if len(clusters) != nrClusters: 
				currentNotClustered +=1
				print()
				print(f"--------------------------------------")
				print(f"Graph [{graph[0]}][{graph[1]}] Does not have {nrClusters} clusters!")
				print(f"Has {len(clusters)}")
				print(f"--------------------------------------")
				print()
				file.write("\n")
				file.write(f"--------------------------------------\n")
				file.write(f"Graph [{graph[0]}][{graph[1]}] Does not have {nrClusters} clusters!\n")
				file.write(f"Has {len(clusters)}\n")
				file.write(f"--------------------------------------\n")
				file.write("\n")
				
				if i == runs: # every time all runs of 1 graph have been counted
					# this needs to be here, otherwise if the last run of a graph has the wrong amount of clusters it will be skipped
					print()
					for x in range(1, nrClusters + 1):
						print(f"Nodes in {x} clusters: {overlapCounts[x]} / {(overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))} = {overlapCounts[x] / (overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))}")
						file.write(f"Nodes in {x} clusters: {overlapCounts[x]} / {(overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))} = {overlapCounts[x] / (overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))}\n")
						totalOverlap[x] += overlapCounts[x]
						overlapCounts[x] = 0
					file.write(f"\n \n")
					print(f"Unused nodes: {len(unusedNodes)}")
					file.write(f"Unused nodes: {len(unusedNodes)}\n")
					for node in unusedNodes:
						print(f" {node}")
						file.write(f" {node}")
					
					file.write(f"\n \n")
					for k in range(nrClusters):
						for j in range(k + 1, nrClusters):
							print(f"Overlap between cluster {k} and cluster {j}: {currentOverlap[k][j]}")
							print(f"Overlapping nodes: ")
							print(*currentOverlapSets[k][j])
							file.write(f"Overlap between cluster {k} and cluster {j}: {currentOverlap[k][j]}\n")
							file.write(f"Overlapping nodes: \n")
							for x in currentOverlapSets[k][j]:
								file.write(str(x))
							file.write("\n \n")	
							overlap[k][j] += currentOverlap[k][j]
							currentOverlap[k][j] = 0
					print()
					file.write(f"\n")
					
					

					notClustered += currentNotClustered
					
					unusedNodes.clear()
					i = 0
					currentNotClustered = 0
				
				continue
			
			# count the overall number of overlaps
			for node in uniqueRange:
				# Count how many clusters "node" is in
				clusterCount = sum(1 for cluster in clusters if node in cluster)
				
				# Update the appropriate counter based on the cluster count
				overlapCounts[clusterCount] += 1
				if clusterCount == 0:
					unusedNodes.append(node)
					
			# count the specific overlap between clusters
			for k in range(nrClusters):
				for j in range(k + 1, nrClusters):
					currentOverlap[k][j] += len(set(clusters[k]).intersection(clusters[j]))
					currentOverlapSets[k][j] = set(clusters[k]).intersection(clusters[j])
					
			
			
			
			if i == runs: # every time all runs of 1 graph have been counted
				print()
				for x in range(1, nrClusters + 1):
					print(f"Nodes in {x} clusters: {overlapCounts[x]} / {(overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))} = {overlapCounts[x] / (overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))}")
					file.write(f"Nodes in {x} clusters: {overlapCounts[x]} / {(overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))} = {overlapCounts[x] / (overlapRatio[x] * comb(nrClusters, x) * (runs - currentNotClustered))}\n")
					totalOverlap[x] += overlapCounts[x]
					overlapCounts[x] = 0
				file.write(f"\n \n")
				print()
				
				print(f"Unused nodes: {len(unusedNodes)}")
				file.write(f"Unused nodes: {len(unusedNodes)}\n")
				print(*unusedNodes)
				for node in unusedNodes:
					file.write(f" {node}")
					
				file.write(f"\n \n") 
					
				for k in range(nrClusters):
					for j in range(k + 1, nrClusters):
						print(f"Overlap between cluster {k} and cluster {j}: {currentOverlap[k][j]}")
						print(f"Overlapping nodes: ")
						print(*currentOverlapSets[k][j])
						file.write(f"Overlap between cluster {k} and cluster {j}: {currentOverlap[k][j]}\n")
						file.write(f"Overlapping nodes: \n")
						for x in currentOverlapSets[k][j]:
							file.write(f"{str(x)} ")
						file.write(f"\n \n")	
						print()
						
						overlap[k][j] += currentOverlap[k][j]
						currentOverlap[k][j] = 0
						currentOverlapSets[k][j].clear()
				print()
				file.write(f"\n")
				
				unusedNodes.clear()
				
				notClustered += currentNotClustered
				i = 0
				currentNotClustered = 0
				
				
				
			
		numGraphs = (graphs * runs) - notClustered
		print()
		file.write("\n")
		if (numGraphs == 1):
			quit()
		
		for x in range(1, nrClusters + 1):
			print(f"Nodes in {x} clusters: {totalOverlap[x]} / {(overlapRatio[x] * comb(nrClusters, x)) * numGraphs} = {totalOverlap[x] / ((overlapRatio[x] * comb(nrClusters, x)) * numGraphs)}")
			
			file.write(f"Nodes in {x} clusters: {totalOverlap[x]} / {(overlapRatio[x] * comb(nrClusters, x)) * numGraphs} = {totalOverlap[x] / ((overlapRatio[x] * comb(nrClusters, x)) * numGraphs)}\n")
		print()
		file.write("\n")
		for k in range(nrClusters):
			for j in range(k + 1, nrClusters):
				print(f"Overlap between cluster {k} and cluster {j}: {overlap[k][j]}")
				file.write(f"Overlap between cluster {k} and cluster {j}: {overlap[k][j]}\n")
		
		print(f"Has {notClustered} / {graphs * runs} not clustered.")
		print()
		file.write(f"Has {notClustered} / {graphs * runs} not clustered.\n")
			
			
				
		


if len(sys.argv) != 7 and len(sys.argv)!= 6:
	print(usage)
	quit()

inputFile = sys.argv[1]
graphs = int(sys.argv[2])
runs  = int(sys.argv[3])
clusterSize = int(sys.argv[4])
nrClusters = int(sys.argv[5])

if len(sys.argv) == 7:
	outputFile = sys.argv[6]
else: 
	outputFile = ""

clusters = readClusterFile(inputFile)
calculateMisclassifications(clusters, graphs, runs, clusterSize, nrClusters, outputFile)