# Microsoft Azure Service Fabric 10.0 Release Notes

* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions
The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Ubuntu 20 <br> Windows | 10.0.1728.1 <br> 10.0.1728.1 <br> 10.0.1816.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 10.0.1816.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 7.0.1816  <br> 10.0.1816 <br> 7.0.1816 <br> 7.0.1816 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |


## Service Fabric Feature and Bug Fixes
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Containers | **Brief Description**: Container image deletion is a process that facilitates image cleanup, regardless of how the images are defined - whether hard-coded in the service manifest or parameterized during application deployment. To enable image pruning, use the "ContainerImageDeletionEnabled" setting in the Hosting configuration. Although the older "PruneContainerImages" setting will still be supported, it cannot be used simultaneously with "ContainerImageDeletionEnabled". <br> **Documentation Reference**: Read more about it at - <br> [Container Image Management](https://learn.microsoft.com/en-us/azure/service-fabric/container-image-management#container-image-management-v2)
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Diagnostics | **Brief Description**: Additional information is surfaced during upgrades, enabling customers to access details about the current health check phase, the duration spent in the phase, and the number of health check flips leading to a retry. <br>**Impact**: This enhancement offers improved clarity during the upgrade domain health check process.<br> **Documentation Reference**: <br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Extensible Storage Engine(ESE) | **Brief Description**: The existing ESE.dll (version 15.0.805.2) is updated with new version of ESE.dll(version 15.0.805.3), in this release.
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Key Value Store(KVS) | **Brief Description**: When building new idle replicas in KVS, the database file is either physically copied or logically copied to the secondary replica. To support future upgrades of the ESE.dll to a new version, there might be instances where the secondary replica's ESE format version is incompatible with the primary replica, leading to potential issues during cluster upgrade/downgrade. To address this, the ESE format version is detected and sent as part of the Copy Context message during the initial handshake of the build replica process.<br>**Feature**: This feature prevents potential incompatibility issues during future SF runtime upgrades/downgrades, ensuring a smoother and error-free experience for customers.<br> **Solution**: The ESE format version field has been added to the Copy Context message sent from the secondary to the primary during the handshake. The secondary passes this information in the copy context, and if the field is not present, a default version of 0 is assumed for the secondary format, ensuring compatibility with existing releases. 
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Extensible Storage Engine(ESE) | **Brief Description**: The legacy auto-compaction settings, "CompactionThresholdInMB", "FreePageSizeThresholdInMB", and "CompactionProbabilityInPercent", has been retired and will no longer be used for determining auto-compaction at open. Instead, a new setting, "FreePageSizeThresholdInPercent", introduced in versions 9.1CU3 and 9.0CU9, will be used for determining auto-compaction at open. **Breaking Change**: Any pre-existing overrides of the legacy auto-compaction settings will be dismissed. <br> **WorkAround**: To fine-tune the frequency and conditions of auto-compaction, kindly utilize the "FreePageSizeThresholdInPercent" setting. <br>**Documentation Reference**: Read more about it at - <ul><li>[FreePageSizeThresholdInPercent](https://learn.microsoft.com/en-us/dotnet/api/system.fabric.localesestoresettings.freepagesizethresholdinpercent?view=azure-dotnet)</li></ul><br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Extensible Storage Engine(ESE) | **Brief Description**: The legacy defragmentation setting, "DefragThresholdInMB," has been phased out for determining defragmentation occurrences. Instead, regular defragmentation will be triggered, and its frequency can be fine-tuned using the "MaxDefragFrequencyInMinutes" setting. Regular defragmentation will take place independent of the database size, leading to more precise calculations for the physical and logical database size, critical for auto-compaction determinations. <br>  **Breaking Change**: Any existing overrides of the "DefragThresholdInMB" setting will be disregarded. <br> **WorkAround**: Adjust defragmentation frequency by utilizing the "MaxDefragFrequencyInMinutes" setting.<br>**Documentation Reference**: Read more about it at - <br> <ul><li>[MaxDefragFrequencyInMinutes](https://learn.microsoft.com/en-us/dotnet/api/system.fabric.localesestoresettings.maxdefragfrequencyinminutes?view=azure-dotnet) </ul></li><br> 
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Extensible Storage Engine(ESE) | **Brief Description**: The settings, "IntrinsicValueThresholdInBytes" and "DatabasePageSizeInKB", have been retired and are not configurable by the user. ESE sets its own value depending on database page size.<br> **Breaking Change**: Any prior overrides of the "IntrinsicValueThresholdInBytes" and "DatabasePageSizeInKB" settings will be disregarded.<br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Lease | **Brief Description**: When a node encounters resource shortages such as insufficient memory or ephemeral ports, it generates system failure reports involving memory allocation, file operations, connection establishment, and inter-node communication. These triggered reports expedite diagnosis during resource-related incidents and contribute to the automatic mitigation of gray nodes.<br>**Feature**: In situations where a node faces resource deficiencies (e.g., limited ephemeral ports for connections or insufficient memory for operations), corresponding reports will be generated. This will effectively reduce diagnosis time, representing a significant stride towards the auto-mitigation of gray nodes. <br> **Solution**: This involves employing a set of probes that periodically assess resource availability within the node. Every 10 seconds, the node attempts to allocate 1 MB of memory, performs open-write-close operations on a file, pings its listening port (Federation), and pings the listening ports of neighboring nodes (Federation and lease). If consecutive failures surpass 2 times or a failure occurs once per minute within a 5-minute interval, the node generates health warnings. (Note: Probes involving network connections require a solid 15 minutes of failure for triggering health warnings.).<br> **Documentation Reference**: Read more about it at - <br> <ul><li>[Degraded node detected](https://github.com/Azure/Service-Fabric-Troubleshooting-Guides/blob/master/Cluster/Why%20is%20my%20node%20degraded.md)</li><li>[Unresponsive neighbor detected](https://github.com/Azure/Service-Fabric-Troubleshooting-Guides/blob/master/Cluster/Why%20is%20my%20cluster%20reporting%20an%20unresponsive%20neighbor.md)</li></ul>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Failover Manager | **Brief Description**: Safety checks are corrected when minimumInstanceCount is equal to targetInstanceCount and/or minimumInstancePercentage is set to 100%.<br> **Solution**: When minimumInstanceCount set to be equal to targetInstanceCount (or minimumInstancePercentage set to 100&), Service Fabric will now create an additional instance before taking down the existing one. This ensures Service Fabric honors the safety definition (minimumInstanceCount and/or minimumInstancePercentage) provided in the Service Description. This will minimize the disruption during planned events like upgrades/repairs <br> **Breaking Change**: If a partition is configured with 5 instances and minimumInstanceCount set to 5, then SF during the upgrades/repairs will maintain a minimum of 5 instances in order to provide availability. Today without this feature, SF will bring the instance count to 4 (not honor the minimumInstanceCount value).<br> **Workaround**: User would need to account for an additional instance in their partition to account for this gap. For example when a partition is configured with 5 instances and a minimumInstanceCount of 5 needs to be maintained, they would need to update the service to bump the instance count to 6 during upgrades/repairs.<br>**Documentation Reference**: For more details check here:<br> [MinInstancePercentage](https://learn.microsoft.com/en-us/dotnet/api/system.fabric.description.statelessservicedescription.mininstancepercentage?view=azure-dotnet&viewFallbackFrom=azure-dotnet%20and%20https%3A%2F%2Flearn.microsoft.com%2Fen-us%2Fdotnet%2Fapi%2Fsystem.fabric.description.statelessservicedescription.mininstancecount%3Fview%3Dazure-dotnet) 
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Backup Restore Service(BRS) | **Brief Description**: Partitions with backup protection may revert to a past restored state upon experiencing data loss. This issue may cause partitions to restore themselves to an unintended or outdated state without user consent or notification and might result in data loss, inconsistency, and corruption, affecting the system's usability and functionality. <br>**Solution**: To address this, partition restore will only occur when an active restore operation has been initiated. If a backup-protected partition encounters data loss, it will wait for user intervention before restoring its state. <br>
| **Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | FabricClient Constructor | **Brief Description**:  The FabricClient constructor addresses a scenario where the provided parameter "hostEndpoints" is empty or null. When the constructor is used in this way, it is assumed that the code is running inside the cluster, and the FabricClient instance will connect to the cluster via the local Gateway service on the same machine using the provided security credentials. <br>**Solution**: Customers can now create a FabricClient instance in their applications that can connect to the local cluster with custom security credentials. The Service Fabric SDK will internally determine the local Gateway endpoint, similar to the behavior of the parameter-less FabricClient constructor. This enhancement provides more flexibility and control for connecting to the local cluster with specific security settings. <br> **Documentation Reference**: Read more about it at - <ul><li>[FabricClient Constructor](https://learn.microsoft.com/en-us/dotnet/api/system.fabric.fabricclient.-ctor?view=azure-dotnet#system-fabric-fabricclient-ctor%28system-fabric-securitycredentials-system-string%28%29%29)</li></ul>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Feature** | Security Audit | **Brief Description**: Service Fabric audits cluster management endpoints' security configurations. The following fields are audited: Action, ServiceName, Port, Protocol, X509StoreLocation, X509StoreName, X509FIndType, X509FindValue, CertificateInUseSHA1. The possible values of Action are "Start, Stop, Update, PeriodicCheck". The possible values of ServiceName are "HttpGateway, EntreeServiceProxy". User's endpoints will not be audited. <br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Bug** | Diagnostics | **Brief Description**: The DEB package for Ubuntu 18.04 and 20.04 has been updated to include "lttng-tools" as a dependency of Service Fabric, and the "lttng-modules-dkms" package has been removed as a dependency.<br>**Feature/Impact**: Due to the optional nature of the "lttng-tools" package in the "lttng-modules-dkms" dependency, there was a possibility that "lttng-tools" would not be installed with Service Fabric. This led to the malfunctioning of internal Service Fabric traces without "lttng-tools". <br> **Solution**:  To address this issue, the "lttng-tools" package has been added as a mandatory dependency to the Service Fabric package. <br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Bug** | Reliable Collection(RC) | **Brief Description**: Enumeration operation in Reliable Collection (RC) occasionally throws ArgumentException, impacting current managed RC users that use enumeration. <br>**Solution**: The issue has been addressed by relaxing the constraint on the internal data structure to allow duplicated keys in a non-steady state.<br>**Workaround**: Implement a try-catch block to retry the enumeration operation with exponential backoff.<br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Bug** | Backup Restore Service | **Brief Description**: When a replica with scheduled backup configured using Backup Restore Service (BRS) encounters OnDataLossAsync, it waits for user intervention to become available. However, this state is not communicated accurately to users through events, health reports, or exception messages. <br>**Solution**: This has been addressed by adding events and exception messages thereby providing an accurate description of the state of the replica to the user along with suggesting steps to make the replica healthy. <br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Bug** | Backup Restore Service(BRS) | **Brief Description**: The "GetBackups" API of Backup Restore Service (BRS), operates asynchronously. However, a part of the API makes a synchronous call to enumerate backup data from Azure Blob Storage which would continue retrieving backups even if the client has timed out. This could result in long-running threads in BRS service especially when it is under load. <br>**Solution**: The fix addresses this issue by converting the Azure Blob Storage call to an asynchronous operation ensuring proper enforcement of API timeouts. As a result, the performance of the GetBackups API is improved significantly. <br>
|**Windows - 10.0.1816.9590<br>Ubuntu 18 - 10.0.1728.1<br>Ubuntu 20 - 10.0.1728.1** | **Bug** | Backup Restore Service(BRS) | **Brief Description**: Presently, in the process of deleting expired backups from the backup store, Backup Restore Service (BRS) attempts to retrieve all backups before initiating the deletion of expired backups. However, this approach can encounter repeated failures if the Backup Store is facing pressure and unable to provide all backups to BRS at once. Consequently, this situation prevents BRS from deleting any expired backups, resulting in a continuous accumulation of backup files.  <br>**Solution**: BRS has been updated to perform the removal of expired backups in batches. The process involves the sequential processing of backups within distinct batches housed in the Backup Store. Within each batch, expired backups will be deleted prior to advancing to the subsequent batch. <br> 


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
|Service Fabric Runtime |Ubuntu Developer Set-up | 10.0.1728.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 10.0.1816.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.10.0.1816.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 10.0.1816.9590 |N/A |https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/10.0.1816.9590/Microsoft.Azure.ServiceFabric.WindowsServer.10.0.1816.9590.zip|
||Service Fabric Standalone Runtime | 10.0.1816.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/10.0.1816.9590/MicrosoftAzureServiceFabric.10.0.1816.9590.cab |
|.NET SDK |Windows .NET SDK | 7.0.1816 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.0.1816.msi |
||Microsoft.ServiceFabric | 10.0.1816 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 7.0.1816 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 7.0.1816 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 10.0.1816 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 10.0.1816 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.1 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |