# Experiment-1: Clustering #

## Purpose ##
Investigate how k-means clustering performs on the image patches. Questions that should be sought answered by the experiment are

* How does the clustering algorithm scale?
  - Influence of k
  - Influence of number of samples

* How does the k parameter influence stability?

* Is there a clear interpretation of the clusters
  - Are instances clustered to originating bags?
  - Are instances clustered to lung regions?




## Data ##
### Dataset-0 ###
10 scans selected randomly from 20 scans used for my bachelor project (`copd_label_data`).
Each scan is represented as a set of feature images. We use the following features 
* Gaussian blur
* Gradient magnitude
* Eigenvalues of the Hessian ordered by magnitude
* Laplacian of Gauss (Sum of eigenvalues)
* Gaussian curvature (Product of eigenvalues)
* Frobenius norm of the Hessian ( Euclidean 2-norm of eigenvalue vector )

All features are calculated at scales {0.6 sqrt(2)^i}_{i=0,1,...6} mm.
Normalized convolution and finite differences are used to blur and estimate derivatives.

### Dataset-1 ###
All features are used.

A large dataset generated by sampling a large number of ROIs from each scan in Dataset-0. The dataset should be of the same size as the actual dataset that will be used for training.

We plan to use 200 scans and sample 50 ROIs from each scan for training. So we need to generate 1000 ROIs from each of the 10 scans in Dataset-0 to match the size.

Due to diskspace constraints only 500 ROIs where sampled from each scan.



### Dataset-2 ###
All features are used.

A small dataset generated by sampling the number of ROIs that will be used for training from Dataset-0.

We plan to use 50 ROIs from each scan for training.



## Experiments ##

### Scalability ###
Run the `KMeansClusterer` several times with increasing k and increasing number of samples from Dataset-1. Measure computation time and memory usage.

#### Data ####
Dataset-1

#### Parameters ####
* Number of clusters = [4, 16, 64]
* Number of samples = [500, 1000, 2500, 5000]
* Number of burnin iterations  = 10
* Nuber of measurement iterations = 100

`KMeansClusterer` implementation parameters
* `branching` is set to the number of clusters
* `iterations` is set to default value (= 11)
* `centers_init` is set to `CENTERS_KMEANSPP`


#### Pseudocode ####
	for k in [4, 16, 64]
	  for n in [500, 1000, 2500, 5000]
	    for i in [1 .. 100]
		  run KMeansClusterer with k clusters on n ROIs and discard measurement
	    for i in [1 .. 1000]
		  run KMeansClusterer with k clusters on n ROIs and record measurement

#### Output ####
Graphs showing computation time as function of number of clusters and number of samples.

#### Extra experiments ####
##### Scalability 2 #####
First run of Scalability show big improvement in running time going from 4 to 16 clusters/branching, and small improvement from 16 to 64.
We want more datapoints to get better estimate of the curve, so we rerun with clusters = [8, 32] and otherwise use the same parameters.

##### Scalability 3 #####
Fix branching at 16 and look at increasing number of clusters.
Remaining parameters are unschanged.


##### KMeans profiling #####
Run the kmeans clustering with valgrind to profile the performance characteristics.
Uses parameters
* branching = k = 32
* 2500 samples




### Stability of clusters ###
Estimate the stability of the clustering for varying number of clusters. The Hasudorff distance, with earth movers distance defining the metric space, is used as a measure of stability.

Two experiments.
* One with clusters = [2, 4, 6, 8, 10, 16, 32, 64] and branching = 2
* One with clusters = [8, 16, 32, 64] and branching = 8

#### Data ####
Dataset-2.

#### Parameters ####
* Number of clusters = [2, 4, 6, 8, 10, 16, 32, 64] and [8, 16, 32, 64]
* Number of iterations = 100
* Number of samples = 500

`KMeansClusterer` implementation parameters
* `branching` is set to [2, 8] 
* `iterations` is set to default value (= 11)
* `centers_init` is set to `CENTERS_KMEANSPP`


#### Pseudocode ####
	for k in [4, 16, 64]
	  for i in [1 .. 1000]
	    run KMeansClusterer with k clusters and store centroids
	  calculate Hausdorff distance between all pairs of k centroids
	  

#### Output ####
Boxplot showing the distribution of Hausdorff distances for each k.




### Interpretation of clusters ###
Depends on the conclusion from the stability experiment.
Investigate if clusters have a simple interpretation. We look at three scenarios

* Will two clusters give us interior/border clusters?
* Will six clusters give us lung region clusters?
* Will ten clusters give us scan clusters?

Each instance is labelled according to the scenarios and entropy is used as a measure of how much the clustering is correlated with the scenarios.


#### Data ####
Dataset-2

#### Parameters ####
* Number of clusters = [2, 6, 10]
* Number of iterations = 1000
* Number of samples = 500

`KMeansClusterer` implementation parameters
* `branching` is set to the number of clusters
* `iterations` is set to default value (= 11)
* `centers_init` is set to `CENTERS_KMEANSPP`

#### Pseudocode ####
	for k in [2, 6, 10]
	  for i in [1 .. 1000]
	    run KMeansClusterer with k clusters and calculate entropy

#### Output ####
Boxplot showing distribution of entropy for each k
