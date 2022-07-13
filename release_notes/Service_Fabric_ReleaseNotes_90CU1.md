# Microsoft Azure Service Fabric 9.0 Cumulative Update 1.0 Release Notes

This release includes bug fixes as described in this documents.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 9.0.1035.1 <br>  9.0.1035.1 <br> 9.0.1028.9590  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 9.0.1017.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 6.0.1028  <br> 9.0.1028 <br> 9.0.1028 <br> 9.0.1028 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |


## Contents 

Microsoft Azure Service Fabric 9.0 Cumulative Update 1.0 Release Notes

* [Current Breaking Changes](#current-breaking-changes)
* [Future Retirement and Deprecation Path Callouts](#future-retirement-and-deprecation-path-callouts)
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

## Future Retirement and Deprecation Path Callouts
* Service Fabric ASP.NET Core packages built against ASP.NET Core 1.0 binaries are out of support. Starting Service Fabric 8.2, we will be building Service Fabric ASP.NET Core packages against .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, .NET Framework 4.6.1. As a result, they can be used only to build services targeting .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, >=.NET Framework 4.6.1 respectively.
For .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, Service Fabric ASP.NET Core will be taking dependency on Microsoft.AspNetCore.App shared framework; whereas, for NetFx target frameworks >=.NET Framework 4.6.1, Service Fabric ASP.NET Core will be taking dependency on ASP.NET Core 2.1 packages.
The package Microsoft.ServiceFabric.AspNetCore.WebListener will no longer be shipped, Microsoft.ServiceFabric.AspNetCore.HttpSys package should be used instead.
* .NET Core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET Core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET Core apps.  This has no impact on Service Fabric .NET Framework SDK.
* .NET 5.0 runtime is reaching end-of-life on May 8, 2022.Service Fabric releases after that date will drop support for Service Fabric applications running with .NET 5.0 runtime.Current applications running on .NET 5.0 runtime will continue to work, but requests for investigations or request for changes will no longer be entertained. Please migrate to using .NET 6.0 version instead.
*  Ubuntu 16.04 LTS reached its 5-year end-of-life window on April 30, 2021.Service Fabric runtime has dropped support for 16.04 LTS, and we recommend moving your clusters and applications to Ubuntu 18.04 LTS or 20.04 LTS.Current applications running on it will continue to work, but requests for investigations or requests for change will no longer be entertained. Please migrate to Ubuntu 18.04 or 20.04 instead.
* Service Fabric runtime will soon be stop using BinaryFormatter based remoting exception serialization by default and move to using Data Contract Serilization (DCS) based remoting exception serialization by default.Current applications using it will continue to work as-is, but Service Fabric strongly recommends moving to using Data Contract Serilization (DCS) based remoting exception instead.
* Service Fabric runtime will soon be archiving and removing Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older from the package Download Center.


## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 9.0.1028.9590<br>Ubuntu 16 - 9.0.1035.1<br>Ubuntu 18 - 9.0.1035.1** | **Bug** | Reliability and Security fix |**Brief desc**: Security fixes related to Container logs and runtime dependencies.<br> For container Logs, Service Fabric runtime on Linux will no longer process Service Fabric-generated diagnostic logs emitted by enlightened containerized workloads. On Windows, the logging agent will no longer use the container-mounted log folder to stage metadata files.<br>Read more about it at - <ul><li>[Service Fabric Privilege Escalation from Containerized Workloads on Linux](https://msrc-blog.microsoft.com/2022/06/28/azure-service-fabric-privilege-escalation-from-containerized-workloads-on-linux/)</li></ul> | <br> Customers will need to update any runtime dependencies with newer versions, which include fixes for known/published vulnerabilities.<br>  |


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 9.0.1035.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 9.0.1028.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.9.0.1028.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 9.0.1028.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/9.0.1028.9590/Microsoft.Azure.ServiceFabric.WindowsServer.9.0.1028.9590.zip |
||Service Fabric Standalone Runtime | 9.0.1028.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/9.0.1028.9590/MicrosoftAzureServiceFabric.9.0.1028.9590.cab |
|.NET SDK |Windows .NET SDK | 6.0.1028 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.6.0.1028.msi  |
||Microsoft.ServiceFabric | 9.0.1035 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 9.0.1035 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.0.516 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 9.0.1028 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.0.516 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.1 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |
