# Hosting subsystem

The [Cluster Manager](/src/prod/src/Management/ClusterManager) informs the **Hosting subsystem** (running on each node) which services it needs to manage for a particular node. The Hosting subsystem then manages the lifecycle of the application on that node. It interacts with the [Reliability](/src/prod/src/Reliability#reliability-subsystem) and [Health](/src/prod/src/Management/healthmanager) services to ensure that the replicas are properly placed, healthy, and happy.  

