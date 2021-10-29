# Microsoft Azure Service Fabric 8.2 Release Notes

This release includes the bug fixes and features described in this document. This release includes runtime, SDKs, and Windows Server Standalone deployments to run on-premises.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Windows | 8.2.1124.1 <br> 8.2.1235.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 8.2.1235.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 5.2.1235 <br> 8.2.1235 <br> 8.2.1235 <br> 8.2.1235 |
|Java SDK |Java for Linux SDK | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module <br> SFCTL | 0.3.15 <br> 11.1.0 |


## Contents 

Microsoft Azure Service Fabric 8.2 Release Notes

* [Key Announcements](#key-announcements)
* [Current And Upcoming Breaking Changes](#Current-And-Upcoming-Breaking-Changes)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
* [Repositories and Download Links](#repositories-and-download-links)


## Key Announcements
* Expose an API in Cluster Manager to note if upgrade is impactful


## Current Breaking Changes
* **Guest Executable and Container Applications**: Guest executable and container applications created or upgraded in Service Fabric clusters with runtime versions 7.1+ are incompatible with prior Service Fabric runtime versions (e.g. Service Fabric 7.0).<br/>
    Following scenarios are impacted:<br/>
    * An application with guest executables or containers is created or upgraded in an Service Fabric 7.1+ cluster.<br/>
    The cluster is then downgraded to a previous Service Fabric runtime version (e.g. Service Fabric 7.0).<br/>
    The application fails to activate.<br/>
    * A cluster upgrade from pre-SF 7.1 version to Service Fabric 7.1+ version is in progress.<br/>
    In parallel with the Service Fabric runtime upgrade, an application with guest executables or containers is created or upgraded.<br/>
    The Service Fabric runtime upgrade starts rolling back (due to any reason) to the pre-SF 7.1 version.<br/>
    The application fails to activate.<br/>
    To avoid issues when upgrading from a pre-SF 7.1 runtime version to an Service Fabric 7.1+ runtime version, do not create or upgrade applications with guest executables or containers while the Service Fabric runtime upgrade is in progress.<br/>
    * The simplest mitigation, when possible, is to delete and recreate the application in Service Fabric 7.0.<br/>
    * The other option is to upgrade the application in Service Fabric 7.0 (for example, with a version only change).<br/>
    If the application is stuck in rollback, the rollback has to be first completed before the application can be upgraded again.
* **.NET Core 2.x Support**: .NET Core runtime LTS 2.1 runtime is out of support as of Aug 21, 2021, and .NET Core runtime 2.2 is out of support from Dec 2019. Service Fabric releases after those dates will drop support for Service Fabric apps running with .NET Core 2.1 & .NET Core 2.2 runtimes respectively. Current apps running on .NET Core 2.x runtime will continue to work, but requests for investigations or request for changes will no longer receive support. Service Fabric .NET SDK will take a dependency on .NET runtime 3.x features to support Service Fabric .NET Core apps.
* **Ubuntu 16.04 Support**: Ubuntu 16.04 LTS reached it's 5-year end-of-life window on April 30, 2021. Due to this, Service Fabric runtime has dropped support for 16.04 LTS and we recommend moving your clusters and applications to Ubuntu 18.04 LTS. Current applications running on it will continue to work, but requests for investigations or requests for change will no longer receive support.
* **ASP.NET Core 1.0 Support**: Service Fabric ASP.NET Core packages are built against ASP.NET Core 1.0 binaries which are out of support. Starting Service Fabric 8.2, we will be building Service Fabric ASP.NET Core packages against .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, .NET Framework 4.6.1. As a result, they can be used only to build services targeting .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, >=.NET Framework 4.6.1 respectively.<br>For .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, Service Fabric ASP.NET Core will be taking dependency on Microsoft.AspNetCore.App shared framework, whereas for NetFx target frameworks >=.NET Framework 4.6.1, Service Fabric ASP.NET Core will be taking dependency on ASP.NET Core 2.1 packages.<br>The package Microsoft.ServiceFabric.AspNetCore.WebListener will no longer be shipped, Microsoft.ServiceFabric.AspNetCore.HttpSys package should be used instead.


## Service Fabric Runtime

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.2.1235.9590<br>Ubuntu 18 - 8.2.1124.1** | **Feature** | Expose an API in Cluster Manager to note if upgrade is impactful | **Brief Description**: Added a new ValidateFabricUpgrade API, which validates the parameters of a Fabric Cluster upgrade request and returns the expected service host impact. Impact is assessed based on whether the upgrade is expected to cause Service Fabric's service host to restart when applied to a node, which leads to all services on the node to restart as well. |


## Service Fabric Add-ons Applications

**Introducing FabricHealer (Preview) - Auto-Mitigation in Service Fabric Clusters**

[FabricHealer](https://github.com/microsoft/service-fabric-healer) is a Service Fabric application that attempts to automatically fix a set of reliably solvable problems that can take place in Service Fabric application services, including containers, host virtual machines, and logical disks (scoped to space usage problems only). These repairs mostly employ a set of Service Fabric API calls, but can also be fully customizable (like Disk repair). All repairs are safely orchestrated through Service Fabric’s RepairManager service. Repair configuration is written as Prolog-like logic with [supporting external predicates](https://github.com/microsoft/service-fabric-healer/tree/main/FabricHealer/Repair/Guan) written in C#. This is made possible by a new logic programming library for .NET, [Guan](https://github.com/microsoft/guan). The fun starts when FabricHealer detects supported error or warning states reported by [FabricObserver](https://github.com/microsoft/service-fabric-observer). FabricHealer requires FabricObserver to be running in the same cluster.

We are especially interested in Feedback with respect to the new [Configuration-as-Logic](https://github.com/microsoft/service-fabric-healer/blob/main/Documentation/LogicWorkflows.md) feature. Out of the box, FabricHealer is configured to solve several problems automatically. You do not have to be a logic programmer to use FabricHealer right away. Check out the [scenario-based getting started samples](https://github.com/microsoft/service-fabric-healer/blob/main/Documentation/Using.md) to get going quickly. There are several logic rules supplied by default that span service to VM level auto-mitigation. You just need to change arguments in the rules to match your needs. 

We are also very interested in your feedback with respect to Guan, should you want to dig into its design and implementation and/or use it in other .NET projects. The way FabricHealer employs Guan is just one use case. As a general purpose logic programming library (.NET Standard), Guan can be used to solve any number of logic programming problems for .NET developers.

Please experiment with FabricHealer Preview in your Test clusters and let us know how it goes. Please create issues on the FabricHealer repo. Thank you!



## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 8.2.1124.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 8.2.1235.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.8.2.1235.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 8.2.1235.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/8.2.1235.9590/Microsoft.Azure.ServiceFabric.WindowsServer.8.2.1235.9590.zip |
||Service Fabric Standalone Runtime | 8.2.1235.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/8.2.1235.9590/MicrosoftAzureServiceFabric.8.2.1235.9590.cab |
|.NET SDK |Windows .NET SDK | 5.2.1235 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.5.2.1235.msi |
||Microsoft.ServiceFabric | 8.2.1235 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 8.2.1235 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.ASP.NET Core.*| 8.2.1235 |https://github.com/Azure/service-fabric-ASP.NET Core |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 8.2.1235 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.2.1235 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.1.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15 |
