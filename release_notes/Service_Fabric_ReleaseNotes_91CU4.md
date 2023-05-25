# Microsoft Azure Service Fabric 9.1 Cumulative Update 4.0 Release Notes

* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions
The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Ubuntu 20 <br> Windows | 9.1.1592.1 <br> 9.1.1592.1 <br> 9.1.1799.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 9.1.1799.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 6.1.1799  <br> 9.1.1799 <br> 6.1.1799 <br> 6.1.1799 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |


## Service Fabric Feature and Bug Fixes
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 9.1.1799.9590<br>Ubuntu 18 - 9.1.1592.1<br>Ubuntu 20 - 9.1.1592.1** | **Feature** | Key Value Store (KVS) | **Brief Description**:  Auto-compaction now emits health events at the start and completion, visible in the Service Fabric Explorer, enhancing its monitoring capabilities. These events complement trace data, providing effective visibility into active auto-compaction progress.
| **Windows - 9.1.1799.9590<br>Ubuntu 18 - 9.1.1592.1<br>Ubuntu 20 - 9.1.1592.1** | **Feature** | Key Value Store (KVS) | **Brief Description**: KVS backed partitions, including Actors and System Services, experience quorum loss due to StoreLVIDLimitHit error code (0x80071d4d), indicating the depletion of values for a monotonically increasing ID field. This results in unavailability of the partition until a manual offline maintenance task is executed to resolve the issue.<br>**Solution**: To address the error, KVS replicas now restart automatically and initiate the maintenance task (Extensible Storage Engine database compaction) during the next open. While there may be temporary availability loss in partitions depending on the task duration, no manual intervention is required to mitigate the issue.
| **Windows - 9.1.1799.9590<br>Ubuntu 18 - 9.1.1592.1<br>Ubuntu 20 - 9.1.1592.1** | **Feature** | Key Value Store (KVS) | **Brief Description**: The "IntrinsicValueThresholdInBytes" setting controls the size of long values stored in a record. If a value exceeds this size, it is stored as an Long Value ID (LVID) within the record. Presently, the default value is 0 which sets the size to 1024 bytes. However, in the future, the value will be changed to 5120 bytes to enable storage of long values up to approximately 5KB within a record. This change is expected to minimize LVID creation and mitigate the risk of LVID exhaustion. <br>**Solution**: The default value of the IntrinsicValueThresholdInBytes is set to 5120 bytes.
| **Windows - 9.1.1799.9590<br>Ubuntu 18 - 9.1.1592.1<br>Ubuntu 20 - 9.1.1592.1** | **Feature** | Application Model | **Brief Description**: Service Fabric has a restriction that allows only one application type with the same name to be provisioned or unprovisioned at any given time. However, if an application type contains illegal path characters in its name or version, it becomes stuck in an unprovisioning loop. This situation not only affects the problematic application type but also causes other application types with the same name to fail during provisioning. <br>**Solution**:  Improvements were made in Service Fabric to address the issue with invalid characters. 
| **Windows - 9.1.1799.9590<br>Ubuntu 18 - 9.1.1592.1<br>Ubuntu 20 - 9.1.1592.1** | **Feature** | Failover Manager | **Brief Description**:  When MinInstancePercentage is set to 100 and the stateless service has N instances, Service Fabric creates additional instances before taking any instances down for upgrade to ensure a total of N instances remain active throughout the upgrade process. However, during the upgrade, if FailoverManager.RelaxCheckForSafeReplicaCloseCount is set to True, only N-1 instances are guaranteed to be available. On the other hand, if FailoverManager.RelaxCheckForSafeReplicaCloseCount is set to False, the upgrade gets stuck in the preSafetyCheck phase, resulting in timeouts and rollbacks due to the unavailability of N instances <br>**Solution**: Set parameters RelaxCheckForSafeReplicaCloseCount as False and IsStrongMinInstanceCountCheckEnabled as True under FailoverManager section in ClusterManifest to move the upgrade forward.  

## Retirement and Deprecation Path Callouts
* As aligned with [Microsoft .NET and .NET Core - Microsoft Lifecycle | Microsoft Learn](https://learn.microsoft.com/en-us/lifecycle/products/microsoft-net-and-net-core), SF Runtime has dropped support for Net Core 3.1 as of December 2022. For  supported versions see [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#current-versions-1) and migrate applications as needed.
* Ubuntu 18.04 LTS will reach its 5-year end-of-life window in June, 2023. Service Fabric runtime will drop support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#supported-linux-versions-and-support-end-date)
* Service Fabric runtime will soon stop supporting BinaryFormatter based remoting exception serialization by default and move to using Data Contract Serialization (DCS) based remoting exception serialization by default. Current applications using it will continue to work as-is, but Service Fabric strongly recommends moving to using Data Contract Serialization (DCS) based remoting exception instead.
* Service Fabric runtime will soon be archiving and removing Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.
* Migrate Azure Active Directory Authentication Library (ADAL) library to Microsoft Authentication Library (MSAL) library, since ADAL will be out of support after December 2022. This will impact customers using AAD for authentication in Service Fabric for below features:<ul><li>Powershell, StandAlone Service Fabric Explorer(SFX), TokenValicationService</li><li>FabricBRS using AAD for keyvault authentication</li><li>KeyVaultWrapper</li><li>ms.test.winfabric.current test framework</li><li>KXM tool</li><li>AzureClusterDeployer tool</li></ul>For more information see: [MSAL Migration] (https://learn.microsoft.com/en-us/azure/active-directory/develop/msal-migration)

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 9.1.1592.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 9.1.1799.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.9.1.1799.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 9.1.1799.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/9.1.1799.9590/Microsoft.Azure.ServiceFabric.WindowsServer.9.1.1799.9590.zip|
||Service Fabric Standalone Runtime | 9.1.1799.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/9.1.1799.9590/MicrosoftAzureServiceFabric.9.1.1799.9590.cab |
|.NET SDK |Windows .NET SDK | 6.1.1799 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.6.1.1799.msi |
||Microsoft.ServiceFabric | 9.1.1799 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 6.1.1799 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 6.1.1799 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 9.1.1799 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 9.1.1799 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.1 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |
