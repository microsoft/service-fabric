# Microsoft Azure Service Fabric 7.0 Third Refresh Release Notes

This release includes the bug fixes and features described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | 7.0.465.1 <br> 7.0.466.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.0.466.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration|4.0.466 <br>7.0.466 <br> 4.0.466 <br> 4.0.466 |
|Java SDK  |Java for Linux SDK  | 1.0.5 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15 <br> 9.0.0 |

## Contents 

Microsoft Azure Service Fabric 7.0 Third Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Repositories and Download Links](#repositories-and-download-links)

## Key Announcements

* **Cluster Upgrade**: We had found regression issues in 7.0 second refresh release rolled out on January 29,2020. Hence, recommend to upgrade to this latest version of 7.0 third refresh release if clusters are on version 6.5 and above.
* **Preview Feature**: [Avoid connection drops during planned downtime of stateless services](https://docs.microsoft.com/azure/service-fabric/service-fabric-application-upgrade-advanced#avoid-connection-drops-during-planned-downtime-of-stateless-services)
* **7.0 breaking change**: **Prevent Application Activations if services have a static port within ApplicationPortRange**. It took effect starting in 7.0 and going forward.There are endpoint resolution conflicts because customer specifies a static port within ApplicationPortRange. This is a breaking change where service activations will FAIL if there is an endpoint with [static port that is within the application port range](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-application-and-service-manifests)


## Service Fabric Runtime

| Versions | IssueType | Description | Resolution | 
|----------|-----------|-|-|
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Bug** | New config CompleteClientRequest in ClusterManager section | **Impact**:  For create/delete app, ClusterManager completes client request until the create/delete operations are finished. If client request has a shorter timeout than ClusterManager, it's possible that client request is timed out but ClusterManager is not and keeps on creating/deleting apps until success. <br>**Fix**:  Add a config CompleteClientRequest in ClusterManager section to allow completing client request when ClusterManager accepts request for create/delete app rather than holds the client request until operation is finished. <br> **Workaround**: N/A |
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Bug** | update application failed with error FABRIC_E_APPLICATION_UPDATE_IN_PROGRESS | **Impact**:  There is a racing problem when ClusterManager fails over during updating app. After ClusterManager is recovered, the pending updating is not picked up by ClusterManager and remains in inprogress status forever, so new update request cannot be accepted. On the other hand, delete app request doesn't clean up update information in ClusterManager, so even if the app is recreated after deleted, it will still show update in progress. <br>**Fix**:  1) Clean up update information in ClusterManager when deleting application, in case there is update in progress that prevents new update app request even when app is recreated after deleted. 2) Recover pending app update context when ClusterManager is recovered. <br> **Workaround**: N/A |
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Bug** | Cluster configuration change may not be notified to Fabric Client. | **Impact**:  Cluster configuration change is notified to configuration change handlers that a service registers to read new configuration values at change. A race condition is found at delivering notification to Fabric client user. For example, the certificate change in Cluster Manifest may not be notified to ImageStoreService and the configuration upgrade could fail by the race condition. <br>**Fix**: The race condition is fixed. <br> **Workaround**: Restart the impacted replica |
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Feature** | ImageStoreService uses custom connection to exchange files. | **Brief desc**:  ImageStoreService uses custom connection in various paths of file exchange. The file delivery includes uploading packages to Service Fabric cluster, downloading package to a node at replica instantiation, and replicating files between ImageStoreService replica. |
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Feature** | **Preview**: Drain requests before closing stateless instances | **Brief desc**:  For planned stateless instance downtime,  when the application/cluster is upgraded or nodes are getting deactivated, there are a few connections drops which are observed because the endpoint exposed by the instances is removed after it goes down. To avoid getting connections drops for planned downtime, configure the replica close delay duration in the service configuration.[**Documentation**](https://docs.microsoft.com/azure/service-fabric/service-fabric-application-upgrade-advanced#avoid-connection-drops-during-planned-downtime-of-stateless-services)|
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Bug** | FabricBRSSetup.exe crashes during cluster upgrade. | **Impact**:  Cluster upgrade can sometimes fail with FabricBRSSetup.exe crashing due to a failure to ACL listener ports, when hosting didn't properly cleanup the same port when it was closing the previous time. Fix has been done to properly cleanup such a port ACLing. <br> **Workaround**: Remove FabricBRS and then upgrade SF cluster. |
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Bug** | Fabric.exe crashes occasionally during initial health reporting. | **Impact**:  Fabric.exe may crash due to a race condition around when it's first brought up. This condition only occurs during the first system health report sent. If the first report is successful, issue will not reoccur for the remainder of the process. <br> **Workaround**: Self healing. Fabric.exe will come back if it crashes and issue will not be hit again until process restart. |
| **Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**  | **Bug** | Cluster health string gives incorrect application and node counts. | **Impact**:  The ClusterHealth ToString method of System.Fabric.dll gives swapped node and application counts. This issue is not reflected in SFX, but can be seen in PowerShell's Get-ServiceFabricClusterHealth API if the returned result is stored and ToString is called, for example. The incorrect string has format "ClusterHealth: {0}, {1} nodes, {2} applications, {3} events". <br> **Workaround**: Swap the node and application counts. |
|**Windows 7.0.466.9590  <br>  Ubuntu 7.0.465.1**   | **Feature** | **Preview** : Support for VMSS OS ephemeral disks | **Brief desc**: Ephemeral OS disks are storage created on the local virtual machine (VM), and not saved to remote Azure Storage. They are recommended for all Service Fabric node types. [**Documentation**](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-azure-deployment-preparation#use-ephemeral-os-disks-for-virtual-machine-scale-sets)

## Repositories and Download Links

The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

|Area |Package | Version | Repository |Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up |7.0.465.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.0.466.9590 | N/A |https://download.microsoft.com/download/a/e/6/ae642d7b-1196-49d8-bacc-c546dd5e12d0/MicrosoftServiceFabric.7.0.466.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package |7.0.466.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.0.466.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.0.466.9590.zip |
||Service Fabric Standalone Runtime |7.0.466.9590 |N/A |https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.0.466.9590/MicrosoftAzureServiceFabric.7.0.466.9590.cab |
|.NET SDK |Windows .NET SDK |4.0.466 |N/A |https://download.microsoft.com/download/a/e/6/ae642d7b-1196-49d8-bacc-c546dd5e12d0/MicrosoftServiceFabricSDK.4.0.466.msi |
||Microsoft.ServiceFabric |7.0.466 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf |4.0.466|https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*|4.0.466 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal |4.0.466 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions |4.0.466 |N/A |https://www.nuget.org |
|Java SDK |Java SDK |1.0.5 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.5 |
|Eclipse |Service Fabric plug-in for Eclipse |2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator |1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator |1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator |1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators |1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI |9.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric |0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
