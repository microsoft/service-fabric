# Reliability

In order to make the services highly reliable, the Reliability subsystem allows creating “secondary” of a “primary” partition and provides replication infrastructure to replicate data between primary and secondary. The reliability subsystem reconfigures and rebalances the primary and secondary of partitions to provide reliability in the event of failures.  

The reliability subsystem stores information about service partitions in a table called failover unit map. In order to ensure consistency of this state, reliability system needs to elect a leader to manage that data and in the event of a failure of a leader it needs some other node in the federation to take over the leadership role and manage that data. Reliability subssystem also needs to be able to know when a node hosting particular replica (primary or secondary) of a service partition is down to be able to reconfigure the replica set to make the application highly reliable. 

The reliability subsystem also needs to be able to communicate with nodes owning a particular replica of a partition for placement and reconfiguration. 
