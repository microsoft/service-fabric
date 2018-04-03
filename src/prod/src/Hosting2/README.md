# Hosting and Activation subsystem

The Cluster Manager informs the hosting subsystem (running on each node) which services it needs to manage for a particular node. The Hosting subsystem then manages the lifecycle of the application on that node. It interacts with the reliability and health components to ensure that the replicas are properly placed and are health and happy.
