# Microsoft Azure Service Fabric 7.2 Sixth Refresh Release Notes

This release includes stability fixes as described in this document.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 7.2.456.1 <br> 7.2.456.1804 <br> 7.2.457.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.2.457.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.2.457 <br> 7.2.457 <br> 4.2.457 <br> 4.2.457 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 10.0.0 |

## Content 

Microsoft Azure Service Fabric 7.2 Sixth Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Upcoming Breaking Changes](#breaking-changes)
* [Service Fabric Runtime](#service-fabric-runtime)

## Key Announcements

* This release introduces a fix to an issue identified in 7.2CU4 and 7.2CU5 in which deleted services were recreated and appear in an error state. With this fix the replica deleted state will now be correctly persisted on the node in all scenarios preventing services from entering an error state upon node restart. If you were impacted by this issue, you can reduce the value of the cluster configuration FailoverManager::DroppedReplicaKeepDuration to mitigate the issue, and update to CU6 to resolve the issue.
* Key Vault references for Service Fabric applications are now GA on Windows and Linux.
* .NET 5 apps for Windows on Service Fabric are now supported as a preview. Look out for the GA announcement of .NET 5 apps for Windows on Service Fabric.
* .NET 5 apps for Linux on Service Fabric will be added in the Service Fabric 8.0 release.

## Breaking Changes

- Service Fabric 7.2 and higher runtime drops support for .NET core Service Fabric apps running with .NET core 2.2 runtime. .NET core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET core LTS version 3.1.

## Upcoming Breaking Changes

- .NET core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET core apps.  This has no impact on Service Fabric .NET Framework SDK.
- Support for Windows Server 2016 and Windows Server 1809 will be discontinued in future Service Fabric releases. We recommend updating your cluster VMs to Windows Server 2019.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.2.457.9590** | **Bug** | Replica deletion state is not correctly persisted on the node |**Brief desc**: In certain scenarios, replica deletion state was not correctly persisted on the node.  <br> **Impact**:This resulted in deleted services being recreated and appearing in error state. <br> **Workaround**:   If you were impacted by this issue, you can reduce the value of the cluster configuration FailoverManager::DroppedReplicaKeepDuration to mitigate the issue, and update to CU6 to resolve the issue. <br> **Fix**: This change ensures that deletion state is persisted correctly. 
| **Windows 7.2.457.9590** | **Bug** | Certain application certificate configurations causing Fabric to crash in 7.1 and 7.2 prior to this release |**Brief desc**: On some SF 7.1 and 7.2 versions, certain Application Certificate configurations would cause Fabric to crash at a regular cadence. <br> **Impact**: Linux Clusters on 7.1 and 7.2 CUs. <br> **Workaround**:  If you hit this scenario, we recommend you upgrade to 7.2 CU5 as soon as possible, otherwise please engage support, you may need to make your application certificate known to the Service Fabric Bootstrap agent extension. <br> **Fix**: This change stabilizes this scenario.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.2.456.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.2.457.9590 | N/A | https://download.microsoft.com/download/7/5/9/759afa24-28ce-4de6-92c0-560244a2d55a/MicrosoftServiceFabric.7.2.457.9590.exe |
| Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.2.457.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.2.457.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.2.457.9590.zip |
||Service Fabric Standalone Runtime | 7.2.457.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.2.457.9590/MicrosoftAzureServiceFabric.7.2.457.9590.cab |
|.NET SDK |Windows .NET SDK | 4.2.457 |N/A | https://download.microsoft.com/download/7/5/9/759afa24-28ce-4de6-92c0-560244a2d55a/MicrosoftServiceFabricSDK.4.2.457.msi |
||Microsoft.ServiceFabric | 7.2.457 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.2.457 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.2.457 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.2.457 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.2.457 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 10.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
