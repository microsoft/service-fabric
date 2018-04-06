# Service Fabric Core Subsystem Architecture

Service Fabric is composed of several subsystems working together to achieve the promise of the platform: to remove the complexities of distributed computing from developers who want to build highly available, resilient, and massively scalable cloud servcices across public and private clouds, and operating systems. 

As you can imagine, it takes a lot of code to materialize Service Fabric. To make it a bit easier for you to navigate through our sources, you can explore our codebase by clicking on a subsystem in the architecural graphic below. As of today, this includes the core subsystems that make up the Service Fabric Standalone product. This will grow over time as we move to open development in this repo, which will include the full product that runs today in Azure, with more components across platform services (e.g., our Chaos Experimentation system and API), build tools, and tests. 

Clicking on an image below will take you to the related subsystem's top level source folder, which will contain introductory information, links to technical design documents and deep dive videos with the folks who write the code you're going to read. 

Enjoy your travels through the code and thank you for jumping in. We really look forward to your contributions and feedback.   
<br/>
 <table>
        <tr>
          <td align="right" width="130">
             <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Management#management-subsystem"><img src="https://github.com/GitTorre/service-fabric/blob/master/docs/architecture/Images/Management_E.png" height="230" width="125" align="right" /></a>
          </td>
          <td align="center" width="564">
              <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Communication#communication-subsystem"><img src="https://github.com/GitTorre/service-fabric/blob/master/docs/architecture/Images/Communication_E.png" /></a> 
            <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Reliability#reliability-subsystem"><img src="https://github.com/GitTorre/service-fabric/blob/master/docs/architecture/Images/Reliability_E.png" /></a>  
              <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Hosting2#hosting-subsystem"><img src="https://github.com/GitTorre/service-fabric/blob/master/docs/architecture/Images/Hosting_E.png" /></a> 
            <br/>
              <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Federation#federation-subsystem"><img src="https://github.com/GitTorre/service-fabric/blob/master/docs/architecture/Images/Federation_E.png" /></a>  
              <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Transport#transport-subsystem"><img src="https://github.com/GitTorre/service-fabric/blob/master/docs/architecture/Images/Transport_E.png" /></a>
          </td>
        <td align="left" width="130">
             <a href="https://github.com/GitTorre/service-fabric/tree/master/src/prod/src/Testability#testability-subsystem"><img src="https://github.com/GitTorre/service-fabric/blob/master/docs/architecture/Images/Testability_E.png" height="230" width="125" align="left" /></a>
          </td>
        </tr>
 </table>
  





