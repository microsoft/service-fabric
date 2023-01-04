# Microsoft Azure Service Fabric 9.1 Cumulative Update 1.0 Release Notes

This release will only be available through manual upgrades. Clusters set to automatic upgrades will not receive this release. For how to configure upgrades, please see [classic](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-upgrade) or [managed](https://docs.microsoft.com/en-us/azure/service-fabric/how-to-managed-cluster-configuration) documentation.

## Contents

* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Key Announcements](#key-announcements)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions
The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Ubuntu 20 <br> Windows | 9.1.1230.1 <br> 9.1.1230.1 <br> 9.1.1436.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 9.1.1436.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 6.1.1436  <br> 9.1.1436 <br> 6.1.1436 <br> 6.1.1436 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |

## Key Announcements

Starting this release (Service Fabric 9.1 Cumulative Update 1.0), the Service Fabric runtime will include a configuration setting that prevents access from Docker containers(deployed as Service Fabric applications) to WireServer endpoint. The setting, "Setup/BlockAccessToWireServer", is supported for Service Fabric clusters deployed on Azure VMs, Windows, and Linux, and defaults to 'false' (access is permitted).  Please note that enabling this setting might break containerized services that rely on access to WireServer - which is uncommon.

## Service Fabric Feature and Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 9.1.1436.9590<br>Ubuntu 18 - 9.1.1230.1<br>Ubuntu 20 - 9.1.1230.1** | **Bug** | Service Fabric DNS | **Brief Description**: Service Fabric DNS names fail to resolve in Process-based services for Windows clusters after upgrading to 9.1.1390.9590 when Hosting.DnsServerListTwoIps set to true in Cluster settings.<br>**Fix**: The code bug is fixed at the backend. Service Fabric Explorer will no longer display the warning message "System.FabricDnsService reported a Warning for property 'Environment.IPv4'. FabricDnsService is not the preferred DNS server on the node."
| **Windows - 9.1.1436.9590<br>Ubuntu 18 - 9.1.1230.1<br>Ubuntu 20 - 9.1.1230.1** | **Bug** | Backup Restore Service (BRS) | **Brief Description**: OnDatalossAsync fails for NetCore applications due to System.MissingMethodException: Method is not found: 'Void System.Fabric.Common.Tracing.FabricEvents.BRSInfoPartitionEvent. This results in the Service partition getting stuck in reconfiguring in the case of data loss or QuorumLoss.<br>**Fix**: Rebuild SF application using SF SDK version 6.0.1107.9590 and upgrade/redeploy applications or Rollback cluster to 9.0.1028.9590 till we have a CU with fix for this regression
| **Windows - 9.1.1436.9590<br>Ubuntu 18 - 9.1.1230.1<br>Ubuntu 20 - 9.1.1230.1** | **Bug** | Backup Restore Service (BRS) | **Brief Description**: If Service Fabric cluster has existing backup policies and is upgraded to 8.2.1686.9590 / 9.0.1107.9590 / 9.1.1387.9590, BRS will fail to deserialize old metadata and will stop taking backup and restore on the partition/service/app in question, even though cluster and BRS remain healthy.<br>**Fix**: The breaking change introduced in 8.2.1686.9590 / 9.0.1107.9590 / 9.1.1387.9590 versions is fixed in this release
| **Windows - 9.1.1436.9590<br>Ubuntu 18 - 9.1.1230.1<br>Ubuntu 20 - 9.1.1230.1** | **Bug** | Cluster Resource Manager (CRM) | **Brief Description**: When the Node properties were removed or updated, the changes are not sent correctly to Failover Manager Master (FMM) resulting in the bad placement of services in the cluster. <br> **Fix**: Support for dynamic Node capacities and Node properties was removed to resolve the issue.
| **Windows - 9.1.1436.9590<br>Ubuntu 18 - 9.1.1230.1<br>Ubuntu 20 - 9.1.1230.1** | **Bug** | Key Value Store (KVS) | **Brief Description**: When the KVS version store reaches its maximum allotted memory due to long-running or especially large uncommitted transactions it would throw a StoreTransactionTooLarge error, which didn't provide accurate information. A new error of "VersionStoreOutOfMemory" was added to provide appropriate information "Version store has exceeded its available memory. This is likely caused by a long-running transaction preventing cleanup, or by a large read/write data load. Version Store Size can be configured with LocalEseStoreSettings.MaxVerPages" to provide accurate information.


## Retirement and Deprecation Path Callouts
* Service Fabric ASP.NET Core packages built against ASP.NET Core 1.0 binaries are out of support. Starting Service Fabric 8.2, we will be building Service Fabric ASP.NET Core packages for .NET Core 3.1, .NET 5.0, .NET 6.0, .NET Framework 4.6.1. As a result, they can be used only to build services targeting .NET Core 3.1, .NET 5.0, .NET 6.0, >=.NET Framework 4.6.1 respectively.<br>
For .NET Core 3.1, .NET 5.0, .NET 6.0, Service Fabric ASP.NET Core will be taking dependency on Microsoft.AspNetCore.App shared framework; whereas, for NetFx target frameworks >=.NET Framework 4.6.1, Service Fabric ASP.NET Core will be taking dependency on ASP.NET Core 2.1 packages.
The package Microsoft.ServiceFabric.AspNetCore.WebListener will no longer be shipped, Microsoft.ServiceFabric.AspNetCore.HttpSys package should be used instead.
* .Net Core 2.1 is out of support as of August 2021. Applications running on .net core 2.1 and lower will stop working from Service Fabric release 10 in Feb 2023. Please migrate to a higher supported version at the earliest.
* .NET 5.0 runtime has reached end-of-life on May 8, 2022. Service Fabric releases after this date will drop support for Service Fabric applications running with .NET 5.0 runtime. Current applications running on .NET 5.0 runtime will continue to work, but requests for investigations or requests for changes will no longer be entertained. Please migrate to using .NET 6.0 version instead.
* Ubuntu 16.04 LTS reached its 5-year end-of-life window on April 30, 2021. Service Fabric runtime has dropped support for 16.04 LTS, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#supported-linux-versions-and-support-end-date)<br>
Applications running on 16.04 will continue to work, but requests to investigate issues w.r.t this OS or requests for change will no longer be entertained. Service Fabric runtime has also dropped producing builds for U16.04 so customers will no longer be able to get SF related patches.
* Ubuntu 18.04 LTS will reach its 5-year end-of-life window on April 30, 2023. Service Fabric runtime will drop support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#supported-linux-versions-and-support-end-date)
* Service Fabric runtime will soon stop supporting BinaryFormatter based remoting exception serialization by default and move to using Data Contract Serialization (DCS) based remoting exception serialization by default. Current applications using it will continue to work as-is, but Service Fabric strongly recommends moving to using Data Contract Serialization (DCS) based remoting exception instead.
* Service Fabric runtime will soon be archiving and removing Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.
* Migrate Azure Active Directory Authentication Library (ADAL) to MSAL library, since ADAL will be out of support after December 2022.This will impact customers using AAD for authentication in Service Fabric for below features:<ul><li>Powershell, StandAlone Service Fabric Explorer(SFX), TokenValicationService</li><li>FabricBRS using AAD for keyvault authentication</li><li>KeyVaultWrapper</li></ul> For more information see: [MSAL Migration] (https://learn.microsoft.com/en-us/azure/active-directory/develop/msal-migration)
* Service Fabric clusters using plain text credentials in the backup policy will have a persistent health warning on Backup Restore Service (BRS) to notify users to make use of the token-based backup storage type. Azure Storage Connection String for the backup storage type is in process of deprecation. From SF 10.0 release (Apr 2023), only token-based backup storage types will be supported.


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 9.1.1230.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 9.1.1436.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.9.1.1436.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 9.1.1436.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/9.1.1436.9590/Microsoft.Azure.ServiceFabric.WindowsServer.9.1.1436.9590.zip |
||Service Fabric Standalone Runtime | 9.1.1436.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/9.1.1436.9590/MicrosoftAzureServiceFabric.9.1.1436.9590.cab |
|.NET SDK |Windows .NET SDK | 6.1.1436 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.6.1.1436.msi  |
||Microsoft.ServiceFabric | 9.1.1436 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 6.1.1436 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 6.1.1436 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 9.1.1436 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 9.1.1436 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.1 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |
