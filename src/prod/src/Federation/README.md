# Federation

The Federation subsystem organizes a set of machines (nodes) into a ring structure that optimally provides highly reliable failure detection, agreement, and leader election among the ring nodes. 

Federation subsytsem is used by the <a href="https://github.com/GitTorre/service-fabric/blob/master/src/prod/src/Reliability/README.md">Reliability subsystem</a> of Service Fabric to make services highly reliable and scalable: The reliability subsystem uses partitioning to achieve scalability; a service is broken down in to partitions that are placed at various nodes on the Federation. 

Federation subsystem is also used by Communication subsystem of Windows Fabric. The <a href="">Communication subsystem</a> provides distributed communication infrastructure that supports various messaging patterns and QOS guarantees. 
