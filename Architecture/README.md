# Service Fabric Architecture

### Service Fabric is a Strongly Consistent platform
From day one, Service Fabric was designed to run strongly consistent internet scale services like Azure Databases. To deliver on the consistency guarantees to its applications, Service Fabric itself is built using layered subsystems where each subsystem provides strong guarantees to its peer and higher level subsystem. For example, the federation subsystem is responsible for managing cluster membership, message routing, leader election, and failure detection with strong guarantees. The reliability system leverages leader election and failure detection capabilities of federation subsystem to build replicated state machines that underlie stateful microservices.  Communication subsystem leverages message routing capability of federation subsystem to reliably deliver its messages. Naming, hosting, and health subsystems work together and depend on the reliability and communication subsystems to manage the lifecycle of the various microservice applications running in the cluster. Our experience running Service Fabric in production for more than a decade has shown that it will be very hard to build a comprehensive distributed systems platform like Service Fabric without properly layering it and thinking about consistency first. Any decently complex internet scale service like Azure DB will have many moving parts and it is similarly hard for it to provide consistency guarantees to its users when its internal componentry is inconsistent. 


#### Service Fabric Subsystems


![Service Fabric Architecture](https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/SFArchitecture.png)



## Service Fabric Architecutre Explorer

Explore the Service Fabric codebase by clicking on an architectural tile below. You'll be taken to the related subsystem's top level source folder start page (README), which will contain introductory information, links to technical design documents, and deep dive videos with the folks who write the code you're going to read. Enjoy!  
<br/>
<br/>
<table style="border: 0px; padding: 0px;">
  <tr>
    <td align="right">
       <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Management/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images//Management.png" height="234" width="184" align="right" /></a>
    </td>
    <td align="left">
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Naming/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Naming.png" /></a> 
      <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Hosting2/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Hosting.png" /></a> 
      <a href="#"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Application.png" /></a> 
      <br/>
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Communication/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Communication.png" /></a> 
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Reliability/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Reliability.png" /></a>
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Federation/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Federation.png" /></a>  
        <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Transport/README.md"><img src="https://github.com/GitTorre/service-fabric/blob/master/Architecture/Images/Transport.png" /></a>
    </td>
  </tr>
</table>
