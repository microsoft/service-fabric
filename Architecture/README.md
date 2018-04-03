# Service Fabric Architecture

From day one, Service Fabric was designed to run strongly consistent internet scale services like Azure Databases. To deliver on the consistency guarantees to its applications, Service Fabric itself is built using layered subsystems where each subsystem provides strong guarantees to its peer and higher level subsystem. For example, the federation subsystem is responsible for managing cluster membership, message routing, leader election, and failure detection with strong guarantees. The reliability system leverages leader election and failure detection capabilities of federation subsystem to build replicated state machines that underlie stateful microservices.  Communication subsystem leverages message routing capability of federation subsystem to reliably deliver its messages. Naming, hosting, and health subsystems work together and depend on the reliability and communication subsystems to manage the lifecycle of the various microservice applications running in the cluster. Our experience running Service Fabric in production for more than a decade has shown that it will be very hard to build a comprehensive distributed systems platform like Service Fabric without properly layering it and thinking about consistency first. Any decently complex internet scale service like Azure DB will have many moving parts and it is similarly hard for it to provide consistency guarantees to its users when its internal componentry is inconsistent. 

Besides taming distributes systems complexity and easing correctness reasoning, putting consistency at the platform layer offers the following efficiencies:  

**Runtime Efficiency**: A layered system like Service Fabric, where consistency primitives are built bottoms up, delivers runtime efficiency in providing consistency with much fewer messages. For example, the cost of failure detection at the federation layer is amortized across 1000s of replicated state machines running in the cluster.  

**Operational Efficiency**: When a livesite issue happens, it is much easier to zero-in on the component that is misbehaving. And, since the application does not deal with consistency primitives, it is easier to isolate failures between platform and the application layers. 

**Developer Efficiency**: Service Fabric is leveraged very broadly inside Microsoft for running many of its cloud infrastructure and services and it is increasingly getting used externally. It isolates infrastructure/service/application developers from dealing having to deal with hard distributed systems problems. If consistency were to be built at the application layer, each distinct application will have to hire distributed systems developers and spend development resources and take longer to reach production quality. Put differently, as comprehensive microservices platform, Service Fabric accords development agility by allowing application developers to focus on their application specific problems by doing the heavy lifting needed to solve the hard distributed systems problems shared by many applications. 

Finally, we believe that it is easy to relax consistency guarantees to arrive at arguably correct weaker consistency models than trying to build consistent applications on a weaker platform. To this effect, Service Fabric provides explicit extension points in its system operation to relax the consistency guarantees built into to enable building weaker-consistency applications. For example, Azure Document DB service, which is also built on top of Service Fabric, leverages these system defined extension points to developer weaker consistency 

#### Service Fabric Architecture

![Service Fabric Architecture](https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/service-fabric-architecture.png)


## Service Fabric Core Subsystem Explorer (Maps top level subsystems to the source code in this repo)

Explore the Service Fabric codebase by clicking on an architectural tile below. You'll be taken to the related subsystem's top level source folder start page (README), which will contain introductory information, links to technical design documents, and deep dive videos with the folks who write the code you're going to read. Enjoy!  
<br/>
<br/>
<table style="border: 0px; padding: 0px;">
  <tr>
    <td align="right">
       <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Management/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Management.png" height="274" width="184" align="right" /></a>
    </td>
    <td align="left">
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Naming/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Naming.png" /></a> 
      <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Hosting2/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Hosting.png" /></a> 
      <br/>
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Communication/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Communication.png" /></a> 
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Reliability/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Reliability.png" /></a>
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Federation/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Federation.png" /></a>  
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Transport/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Transport.png" /></a>
    </td>
  </tr>
</table>
