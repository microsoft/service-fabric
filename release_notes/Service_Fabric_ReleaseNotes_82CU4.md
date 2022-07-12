# Microsoft Azure Service Fabric 8.2 Cumulative Update 4.0 Release Notes

This release includes the bug fixes described in this document. This release includes runtime, SDKs, and Windows Server Standalone deployments to run on-premises.

This release will only be available through manual upgrades. Clusters set to automatic upgrades will not receive this release. For how to configure upgrades, please see [classic](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-upgrade) or [managed](https://docs.microsoft.com/en-us/azure/service-fabric/how-to-managed-cluster-configuration) documentation.


The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 8.2.1458.1 <br>  8.2.1458.1 <br> 8.2.1659.9590  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 8.2.1659.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 5.2.1659  <br> 8.2.1659 <br> 8.2.1659 <br> 8.2.1659 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |


## Contents 

Microsoft Azure Service Fabric 8.2 Cumulative Update 4.0 Release Notes

* [Current Breaking Changes](#current-breaking-changes)
* [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
* [Repositories and Download Links](#repositories-and-download-links)


## Current Breaking Changes

* **Guest Executable and Container Applications**: Guest executable and container applications created or upgraded in Service Fabric clusters with runtime versions 7.1+ are incompatible with prior Service Fabric runtime versions (e.g. Service Fabric 7.0).<br/>
    Following scenarios are impacted:<br/>
    * An application with guest executables or containers is created or upgraded in a Service Fabric 7.1+ cluster.<br/>
    The cluster is then downgraded to a previous Service Fabric runtime version (e.g. Service Fabric 7.0).<br/>
    The application fails to activate.<br/>
    * A cluster upgrade from a version prior to Service Fabric v7.1 to Service Fabric 7.1+ version is in progress.<br/>
    In parallel with the Service Fabric runtime upgrade, an application with guest executables or containers is created or upgraded.<br/>
    The Service Fabric runtime upgrade starts rolling back (due to any reason) to a version prior to Service Fabric runtime v7.1.<br/>
    The application fails to activate.<br/>
    To avoid issues when upgrading from a version prior to Service Fabric runtime v7.1 to a Service Fabric 7.1+ runtime version, do not create or upgrade applications with guest executables or containers while the Service Fabric runtime upgrade is in progress.<br/>
    * The simplest mitigation, when possible, is to delete and recreate the application in Service Fabric 7.0.<br/>
    * The other option is to upgrade the application in Service Fabric 7.0 (for example, with a version only change).<br/>
    If the application is stuck in rollback, the rollback has to be first completed before the application can be upgraded again.

<br>


## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.2.1659.9590<br>Ubuntu 16 - 8.2.1458.1<br>Ubuntu 18 - 8.2.1458.1** | **Bug** | Failover Manager cache | **Brief desc**: Cache cleanup logic of Failover Manager had a bug which resulted in memory increase and performance degradation.<br>**Impact**: This can cause clusters to be extremely unresponsive and slow.<br>**Fix**: . 


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 8.2.1458.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 8.2.1659.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.8.2.1620.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 8.2.1659.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/8.2.1620.9590/Microsoft.Azure.ServiceFabric.WindowsServer.8.2.1620.9590.zip |
||Service Fabric Standalone Runtime | 8.2.1659.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/8.2.1620.9590/MicrosoftAzureServiceFabric.8.2.1620.9590.cab |
|.NET SDK |Windows .NET SDK | 5.2.1659 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.5.2.1620.msi  |
||Microsoft.ServiceFabric | 8.2.1659 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 8.2.1659 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.2.1659 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 8.2.1659 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.2.1659 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.1 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |
