# Service Fabric Architecture


![Service Fabric Architecture](https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/service-fabric-architecture.png)  



Service Fabric is designed to run strongly consistent internet scale services like SQL Azure and Cosmos DB, to name a few. **To deliver on the consistency guarantees to its applications, Service Fabric itself is built using layered subsystems where each subsystem provides strong guarantees to its peer and higher level subsystems**. For example, the **Federation subsystem** is responsible for managing cluster membership, message routing, leader election, and failure detection with strong guarantees. The **Reliability subsystem** leverages leader election and failure detection capabilities of the Federation subsystem to build **replicated state machines** that are the foundation of **stateful** microservices.  The **Communication subsystem** leverages message routing capabilities of the Federation subsystem to reliably deliver its messages. **Naming, Hosting, and Health subsystems** work together and depend on the ***Reliability*** and ***Communication subsystems*** to manage the lifecycle of the various microservice applications running in the cluster.  

**Our experience running Service Fabric in production for more than a decade has shown that it will be very hard to build a comprehensive distributed systems platform like Service Fabric without properly layering it and thinking about consistency** ***first***. Any decently complex internet scale service like Azure DB or Cosmos DB, for example, will have many moving parts and it is similarly hard for it to provide consistency guarantees to its users when its internal componentry is inconsistent. This is precisely why both SQL Azure and Comsmos DB run on Service Fabric. 

Besides taming distributed systems complexity and easing correctness reasoning, Service Fabric is designed to put consistency at the platform layer, which guarantees the following efficiencies:  

**Runtime Efficiency**: **A layered system like Service Fabric, where consistency primitives are built bottoms up, delivers runtime efficiency in providing consistency with much fewer messages**. For example, the cost of failure detection at the federation layer is amortized across 1000s of replicated state machines running in the cluster.  

**Operational Efficiency**: When a livesite issue happens, **it is much easier to zero-in on the component that is misbehaving. And, since the application does not deal with consistency primitives, it is easier to isolate failures between platform and the application layers**. 

**Developer Efficiency**: **Service Fabric is leveraged very broadly inside Microsoft for running many of its cloud infrastructure and services and it is increasingly getting used externally. It isolates infrastructure/service/application developers from dealing having to deal with hard distributed systems problems.** If consistency were to be built at the application layer, each distinct application will have to hire distributed systems developers and spend development resources and take longer to reach production quality. Put differently, as comprehensive microservices platform, Service Fabric accords development agility by allowing application developers to focus on their application specific problems by doing the heavy lifting needed to solve the hard distributed systems problems shared by many applications. 

Finally, **we believe that it is easy to relax consistency guarantees to arrive at arguably correct weaker consistency models than trying to build consistent applications on a weaker platform**. To this effect, Service Fabric provides explicit extension points in its system operation to relax the consistency guarantees built into to enable building weaker-consistency applications. 

### Infrastructure concepts  

***Cluster***: A network-connected set of virtual or physical machines into which your microservices are deployed and managed. Clusters can scale to thousands of machines.  

***Node***: A machine or VM that's part of a cluster is called a node. Each node is assigned a node name (a string). Nodes have characteristics, such as placement properties. Each machine or VM has an auto-start Windows service, FabricHost.exe, that starts running upon boot and then starts two executables: Fabric.exe and FabricGateway.exe. These two executables make up the node. For testing scenarios, you can host multiple nodes on a single machine or VM by running multiple instances of Fabric.exe and FabricGateway.exe.  

### System services  

There are system services that are created in every cluster that provide the platform capabilities of Service Fabric.  

***Naming Service***: Each Service Fabric cluster has a Naming Service, which resolves service names to a location in the cluster. You manage the service names and properties, like an internet Domain Name System (DNS) for the cluster. Clients securely communicate with any node in the cluster by using the Naming Service to resolve a service name and its location. Applications move within the cluster. For example, this can be due to failures, resource balancing, or the resizing of the cluster. You can develop services and clients that resolve the current network location. Clients obtain the actual machine IP address and port where it's currently running.  

***Image Store Service***: Each Service Fabric cluster has an Image Store service where deployed, versioned application packages are kept. Copy an application package to the Image Store and then register the application type contained within that application package. After the application type is provisioned, you create a named application from it. You can unregister an application type from the Image Store service after all its named applications have been deleted.  

***Failover Manager Service***: Each Service Fabric cluster has a Failover Manager service that is responsible for the following actions:  

Performs functions related to high availability and consistency of services.  
Orchestrates application and cluster upgrades.  
Interacts with other system components.  

***Repair Manager Service***: This is an optional system service that allows repair actions to be performed on a cluster in a way that is safe, automatable, and transparent. Repair manager is used in:  

Performing Azure maintenance repairs on Silver and Gold durability Azure Service Fabric clusters.  
Carrying out repair actions for Patch Orchestration Application


## Service Fabric Subsystem Explorer (Maps top level Service Fabric services to the source code in this repo)

Explore the Service Fabric codebase by clicking on an architectural tile below. You'll be taken to the related subsystem's top level source folder start page (README), which will contain introductory information, links to technical design documents, and deep dive videos with the folks who write the code you're going to read. Enjoy!  
<br/>
<br/>

<table style="border: 0px; padding: 0px;" width="600">
  <tr>
    <td align="right">
       <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Management/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Management_D.png" height="284" width="234" align="right" /></a>
    </td>
    <td align="left">
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Communication/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Communication_D.png" /></a> 
      <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Reliability/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Reliability_D.png" /></a>  
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Hosting2/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Hosting_D.png" /></a> 
      <br/>
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Federation/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Federation_D.png" /></a>  
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Transport/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Transport_D.png" /></a>
    </td>
  </tr>
</table>
