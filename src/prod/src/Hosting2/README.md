# Hosting subsystem

The [Cluster Manager](https://github.com/Microsoft/service-fabric/tree/f258f7579af9643dac6b1c75c93db9a3bcd28fdd/src/prod/src/Management/ClusterManager) informs the **Hosting subsystem** (running on each node) which services it needs to manage for a particular node. The Hosting subsystem then manages the lifecycle of the application on that node. It interacts with the [Reliability](https://github.com/Microsoft/service-fabric/tree/master/src/prod/src/Reliability#reliability-subsystem) and [Health](https://github.com/Microsoft/service-fabric/tree/master/src/prod/src/Management/healthmanager) services to ensure that the replicas are properly placed, healthy, and happy.  

