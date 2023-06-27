# Microsoft Azure Service Fabric 9.1 Cumulative Update 5.0 Release Notes

* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions
The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Ubuntu 20 <br> Windows | 9.1.1625.1 <br> 9.1.1625.1 <br> 9.1.1833.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 9.1.1833.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 6.1.1833  <br> 9.1.1833 <br> 6.1.1833 <br> 6.1.1833 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |


## Service Fabric Feature and Bug Fixes
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 9.1.1833.9590<br>Ubuntu 18 - 9.1.1625.1<br>Ubuntu 20 - 9.1.1625.1** | **Feature** | Key Value Store (KVS) | **Brief Description**:  "DatabaseFilesCorrupted" is a terminal error for the replica, requiring it to be dropped. This error occurs when the Extensible Storage Engine(ESE) Write change APIs return the DatabaseFilesCorrupted error, resulting in the replica being marked for compaction and flagged as a fault replica.<br>**Solution**: When a replica experiences database corruption via KVS Write APIs (e.g., Insert, Update, Delete), it is flagged with the repair task "MarkCompactTransientFault". Upon reopening, the JET_Compact API identifies the "DatabaseFilesCorrupted" state, initiating the repair policy that ultimately results in the replica being dropped. This solution ensures proper handling and repair of replicas affected by database corruption, maintaining system integrity.
| **Windows - 9.1.1833.9590<br>Ubuntu 18 - 9.1.1625.1<br>Ubuntu 20 - 9.1.1625.1** | **Bug** | Failover Manager | **Brief Description**: During a scaling-down scenario in Service Fabric, if an application upgrade is in progress, there is a possibility of Service Fabric miscalculating the MinimumInstancePercentage for stateless services. This can result in the application upgrade getting stuck. <br>**Solution**: The issue has been resolved, ensuring that Service Fabric accurately calculates the MinimumInstancePercentage during scaling down. This fix enables the application upgrade to progress smoothly without any disruptions. <br>**Workaround**: To mitigate the issue, set the MinInstancePercentage to 0 and utilize the MinInstanceCount instead.
| **Windows - 9.1.1833.9590<br>Ubuntu 18 - 9.1.1625.1<br>Ubuntu 20 - 9.1.1625.1** | **Bug** | Service Fabric Explorer (SFX) | **Brief Description**: The repair job timeline history has been updated to fix the incorrect ordering of claimed and approved actions. This correction ensures an accurate representation of the repair job actions in the history section, providing clarity for users reviewing the timeline.


## Retirement and Deprecation Path Callouts
* As aligned with [Microsoft .NET and .NET Core - Microsoft Lifecycle | Microsoft Learn](https://learn.microsoft.com/en-us/lifecycle/products/microsoft-net-and-net-core), SF Runtime has dropped support for Net Core 3.1 as of December 2022. For  supported versions see [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#current-versions-1) and migrate applications as needed.
* Ubuntu 18.04 LTS will reach its 5-year end-of-life window in June, 2023. Service Fabric runtime will drop support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#supported-linux-versions-and-support-end-date)
* Service Fabric runtime will soon stop supporting BinaryFormatter based remoting exception serialization by default and move to using Data Contract Serialization (DCS) based remoting exception serialization by default. Current applications using it will continue to work as-is, but Service Fabric strongly recommends moving to using Data Contract Serialization (DCS) based remoting exception instead.
* As called out in the previous releases, Service Fabric runtime will remove Service Fabric runtime version 6.4 packages and older, SDK version 3.3 packages and older from the package Download Center in July 2023. 
* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.<br>
To prevent disruption of workloads, create a new cluster using the following steps:<ul><li>[Create a Service Fabric cluster using ARM template](https://learn.microsoft.com/en-us/azure/service-fabric/quickstart-cluster-template)</li><li>[Create a Standalone cluster](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)</li></ul> Install the supported version of Service Fabric SDK based on the Runtime version installed on the cluster.  

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 9.1.1625.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 9.1.1833.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.9.1.1833.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 9.1.1833.9590 |N/A |https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/9.1.1833.9590/Microsoft.Azure.ServiceFabric.WindowsServer.9.1.1833.9590.zip|
||Service Fabric Standalone Runtime | 9.1.1833.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/9.1.1833.9590/MicrosoftAzureServiceFabric.9.1.1833.9590.cab |
|.NET SDK |Windows .NET SDK | 6.1.1833 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.6.1.1833.msi |
||Microsoft.ServiceFabric | 9.1.1833 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 6.1.1833 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 6.1.1833 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 9.1.1833 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 9.1.1833 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.1 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |
