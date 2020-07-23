# Microsoft Azure Service Fabric 7.0  Release Notes

This release includes the bug fixes and features described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | 7.0.457.1 <br> 7.0.457.9590  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.0.457.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.0.457  <br> 7.0.457 <br> 4.0.457 <br> 4.0.457 |
|Java SDK  |Java for Linux SDK  | 1.0.5 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 8.0.0 |

## Contents 

Microsoft Azure Service Fabric 7.0 Release Notes

* [Key Announcements](#key-announcements)
* [Upcoming Breaking Changes](#upcoming-breaking-changes)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
* [Service Fabric Explorer](#service-fabric-explorer)
* [Reliable Collections](#reliable-collections)
* [Repositories and Download Links](#repositories-and-download-links)
* [Visual Studio 2015 Tool for Service Fabric - Localized Download Links](#visual-studio-2015-tool-for-service-fabric-\--localized-download-links)

## Key Annoucements
-  Service Fabric applications with [Managed Identities](https://docs.microsoft.com/en-us/azure/service-fabric/concepts-managed-identity) enabled can directly [reference a keyvault](https://docs.microsoft.com/azure/service-fabric/service-fabric-keyvault-references) secret URL as environment/parameter variable. Service Fabric will automatically resolve the secret using the application's managed identity.
- **Improved upgrade safety for stateless services**: To guarantee availability during an application upgrade, we have introduced new configurations to define the [minimum number of instances for stateless services](https://docs.microsoft.com/en-us/dotnet/api/system.fabric.description.statelessservicedescription?view=azure-dotnet) to be considered available. Previously this value was 1 for all services and was not changeable. With this new per-service safety check, you can ensure that your services retain a minimum number of up instances during application upgrades, cluster upgrades, and other maintenance that relies on Service Fabric’s health and safety checks.
- [Resource Limits for User Services](https://docs.microsoft.com/azure/service-fabric/service-fabric-resource-governance#enforcing-the-resource-limits-for-user-services): users can setup resource limits for user services on a node to prevent scenarios like resource exhaustion of SF system services.
- Introduction of the [Very High move cost](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-resource-manager-movement-cost) for a replica type. We have introduced a new “Very High” move cost value (above “High”) that provides additional control to prevent unnecessary movements in some usage scenarios.  Please see the docs for additional information on when usage of a “Very High” move cost is reasonable and for additional considerations.
- [ReliableCollectionsMissingTypesTool](https://github.com/hiadusum/ReliableCollectionsMissingTypesTool/releases/tag/1.0.0): This tool helps validate that types used in reliable collections are forward and backward compatible during a rolling application upgrade. This helps prevent upgrade failures or data loss and data corruption due to missing or incompatible types.
- **Configurable seed node quorum safety check**: ability for the users to customize how many seed nodes up have to be available during application lifecycle management scenarios.
- **Service Fabric Explorer**:  Added support for [managing Backup Restore service](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-backuprestoreservice-quickstart-azurecluster#using-service-fabric-explorer). This makes the following activities possible directly from within SFX: discovering backup and restore service, ability to create backup policies, enabling automatic backup for application, service, partitions,taking adhoc backup, triggering a restore operation apart from browsing existing backups.
- **Additional cluster safety checks**: In this release we introduced a configurable seed node quorum safety check. This allows you to customize how many seed nodes must be available during cluster lifecycle and management scenarios. Operations which would take the cluster below the configured value are blocked. Today the default value is always a quorum of the seed nodes, for example, if you have 7 seed nodes, an operation that would take you below 5 seed nodes would be blocked by default. With this change, you could make the minimum safe value 6, which would allow only one seed node to be down at a time.   

## Current And Upcoming Breaking Changes
- [Prevent Service Activations if you have specified a static port within ApplicationPortRange](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabricapplication-and-service-manifests#describe-a-service-in-servicemanifestxml). This is a **breaking change** where service activations will FAIL if there is an endpoint with static port  that is within the application port range.
- For customers [using service fabric to export certificates into their linux containers](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-securing-containers), the export mechanism will be changed in an upcoming CU to encrypt the private key included in the .pem file, with the password being stored in an adjacent .key file.
- Starting in Service Fabric 7.0 version, customers using service fabric managed identities, **please switch to new environment variables ‘IDENTITY_ENDPOINT’ and ‘IDENTITY_HEADER’**. The prior environment variables    
  'MSI_ENDPOINT', ‘MSI_ENDPOINT_’ and 'MSI_SECRET' are now deprecated and will be removed in next CU release.
- Starting in Service Fabric 7.0 version, we have changed the default value for the PreferUpgradedUDs config in the [PlacementAndLoadBalancing section of the ClusterManifest](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings#placementandloadbalancing).Based on numerous cases and customer issues, this default behavior should allow clusters to remain more balanced during upgrades. If you want to preserve today’s behavior for your workload, please perform a cluster configuration upgrade to explicitly set the value to true either prior to or as a part of upgrading your cluster to 7.0.
- Starting in Service Fabric 7.0 version, we have fixed a calculation bug in the Cluster Resource Manager which impacts how node resource capacities are calculated in cases where a user manually provides the values for node resource capacities. The impact of this fix is that the Cluster Resource Manager will now consider there to be less usable capacity in the cluster. More details in the [Resource governance documentation page](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-resource-governance).
- We are planning to update the way Service Fabric performs equality comparison for Boolean values when evaluating placement constraints. Today Service Fabric uses case-sensitive comparison for Boolean values in this scenario. Starting in one of the subsequent releases, we will be changing this comparison to be case-insensitive (which is a common practice in the software industry).
- For customers using [Windows Azure Diagnostics](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-diagnostics-event-aggregation-wad) to collect Reliable Collections events, the Reliable Collections ETW manifest provider has been changed to *02d06793-efeb-48c8-8f7f-09713309a810*, and should be accounted for in the WAD config.
- Currently Service Fabric ships the following nuget packages as a part of our    ASP.Net Integration and support:
    -  Microsoft.ServiceFabric.AspNetCore.Abstractions
    -  Microsoft.ServiceFabric.AspNetCore.Configuration
    -  Microsoft.ServiceFabric.AspNetCore.Kestrel
    -  Microsoft.ServiceFabric.AspNetCore.HttpSys
    -  Microsoft.ServiceFabric.AspNetCore.WebListener

   These packages are built against AspNetCore 1.0.0 binaries which have gone out of support (https://dotnet.microsoft.com/platform/support/policy/dotnet-core). Starting in Service Fabric 8.x we will start building Service Fabric AspNetCore integration against AspNetCore 2.1  and for netstandard 2.0. As a result, there will be following changes:
    1. The following binaries and their nuget packages will be released for     
      netstandard 2.0 only.These packages can be used in applications targeting .net framework <4.6.1 and .net core >=2.0
            -  Microsoft.ServiceFabric.AspNetCore.Abstractions
            -  Microsoft.ServiceFabric.AspNetCore.Configuration
            -  Microsoft.ServiceFabric.AspNetCore.Kestrel
            -  Microsoft.ServiceFabric.AspNetCore.HttpSys
            
    2. The following package will no longer be shipped:
            - Microsoft.ServiceFabric.AspNetCore.WebListener:
                 - Use Microsoft.ServiceFabric.AspNetCore.HttpSys instead.


## Service Fabric Runtime

| Versions | IssueType | Description | Resolution | 
|----------|-----------|-|-|
| **Windows 7.0.457.9590 <br> Ubuntu 7.0.457.1** | **Bug** | The default timeout (10 minutes) for WaitForReconfiguration and WaitForInBuildReplica safety checks during upgrades and node deactivations is too short. | **Impact**: In some scenarios, the failure of WaitForReconfiguration and WaitForInBuildReplica safety checks implies that the upgrade or node deactivation would cause availability loss if it were allowed to proceed. The timeout for these two safety checks is 10 minutes by default. After the timeout period elapses, Service Fabric allows the upgrade or node deactivation to proceed even if the safety checks are failing. In some scenarios, this can lead to availability loss for the partition that is failing the safety check.  <br>**Fix**: The default timeout for the WaitForReconfiguration and WaitForInBuildReplica safety checks has been made infinite to avoid availability loss situations. <br> **Workaround**: Change the default timeout to a very high value using the WaitForReconfigurationSafetyCheckTimeout and WaitForInBuildReplicaSafetyCheckTimeout parameters in the FailoverManager section of cluster settings. https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings#failovermanager |
| **Windows 7.0.457.9590** | **Bug** | Service Fabric Powershell Version isn't upgraded on host due to open handle. | **Impact**: If the Powershell module is loaded and the handle for the associated DLL is held the module would fail to upgrade during a runtime upgrade, leaving the the old module in place. <br> **Fix**: During a Service Fabric upgrade handles holding the powershell module are now closed <br> **Workaround**: Ensuring that the powershell module isn't open during a Service Fabric runtime upgrade |
| **Windows 7.0.457.9590** | **Bug** | Security setting CrlCheckingFlags not used by Federation. | **Impact**: If the user specified a setting for CrlCheckingFlags, it would not be used by Federation. <br>**Fix**: Deprecate the internal override of CrlCheckingFlags <br> **Workaround**: Set internal config setting Federation/X509CertChainFlags to the same value as CrlCheckingFlags |
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | Expose a telemetry from Hosting for Port Exhaustion. | **Brief desc** Expose a telemetry for the application port range when the usage goes beyond a threshold. This will give customers like sql to monitor application port usage and help them avoid issues where package activations fails because of too many applications on the nodes. |
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | Configurable seed node quorum safety check | **Brief desc** New config SeedNodeQuorumAdditionalBufferNodes under FailoverManager section is introduced with a default value of 0. This config represents buffer of seed nodes that needs to be up (in addition to quorum of seed nodes). This means that Service Fabric will allow maximum of (total seed node number - (seed node quorum + SeedNodeQuorumAdditionalBufferNodes)) seed nodes to go down. When changing this config, make sure that its value is greater or equal to 0, and ensure that it doesn't block node deactivation and upgrades (at least one node needs to be allowed to go down).
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | ServicePlacementTimeLimit | **Brief desc** We have introduced new parameter ServicePlacementTimeLimit for Stateful services, which overrides the PlacementTimeLimit parameter from FailoverManager cluster config. ServicePlacementTimeLimit defines how much time can the partitions of the service remain in the InBuild state before an error health report is generated to indicate that the build is stuck. If not specified, the default value is taken from the PlacementTimeLimit parameter in [cluster settings for FailoverManager](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings#failovermanager). This setting is useful when the PlacementTimeLimit parameter in cluster settings for FailoverManager is not appropriate for all services. In such cases, ServicePlacementTimeLimit can be used to specify the expected InBuild duration on a per-service basis. 
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | VeryHigh service move cost | **Brief desc** We have introduced new move cost value VeryHigh that provides additional flexibility in some usage scenarios. For more details please consult the [Service movement cost](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-resource-manager-movement-cost) documentation.
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | Resource Limits for User Services | **Brief desc** Some SF customers encountered situations where runaway user services consumed all the resources on the nodes they were running on. This could result in several bad effects including starving other services on the nodes, nodes ending up in a bad state, complex mitigation and recovery steps, and unresponsive cluster management APIs. With this version of Service Fabric, users can now set up resource limits for the user services on a node. Service Fabric will enforce these hard resource limits for user services to guarantee that all the non-system services on a node will never use more than the specified amount of resources. More details in the [Resource governance documentation page](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-resource-governance).
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | Downloads from image store to nodes using internal transport | **Brief desc** When Service Fabric deploys an application to nodes, if its application package is stored in the image store, the packages is downloaded to each deployed node. Before 7.0, application package contents are downloaded using SMB. Starting with 7.0, to reduce dependency on SMB, by default such downloads use an internal file transfer transport on top of existing fabric transport.
|**Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | Prevent Service Activations if you have specified a static port within ApplicationPortRange| There are endpoint resolution conflicts because customer specifies a static port within ApplicationPortRange. This is a **breaking change** where service activations will FAIL if there is an endpoint with static port  that is within the application port range. [Details]( https://docs.microsoft.com/en-us/azure/service-fabric/service-fabricapplication-and-service-manifests#describe-a-service-in-servicemanifestxml)


## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.0.457.9590** | **Bug** |Improve DNS intermittent resolve errors|**Brief desc**: Intermittent resolve shows up as short periods of time where a resolve will timeout or be unable to correctly statisfy a request.  <br> **Impact**: Causes interruptions in service for customers. <br> **Workaround**: Wait for it to self correct or apply a patch script. <br> **How/When to Consume it**: In fabric settings, set the Hosting parameter DnsServerListTwoIps to true. **Documentation: https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings#dnsservice| 

## Service Fabric Explorer
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | Event store viewer timeline|**Brief desc**: A scrollable timeline has been added to the event store viewer for cluster upgrades, application upgrades and viewing nodes in down state. https://github.com/microsoft/service-fabric-explorer/issues/219| 
| **Windows 7.0.457.9590** | **Feature** | Backup Restore Service Management |**Brief desc**: Support for Backup Restore service management has been added to Service Fabric Explorer. This enables creating new backup policies, enable an application or service or partition for periodic backups, triggering an adhoc backup for any partition or restoring a partition from a previous backup and enumerating existing backups.| 


## Reliable Collections
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** |Reliable Collections Upgrade Compatibility Verification Tool|**Brief desc**:  Developed [ReliableCollectionsMissingTypesTool](https://github.com/hiadusum/ReliableCollectionsMissingTypesTool/releases/tag/1.0.0) that helps validate the types used in reliable collections to prevent upgrade failure and data loss situations. This ensures that the versions are forwards and backwards compatible with each other.- During a rolling application upgrade, the upgrade is applied to a subset of nodes, one upgrade domain at a time. During this process, some upgrade domains are on the newer version of your application, and some upgrade domains are on the older version of your application. During the rollout, the new version of your application must be able to read the old version of your data and vice versa. If the data format is not forward and backward compatible, the upgrade may fail, or worse, data may be lost or corrupted. |
|**Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | IReliableDictionary3 (beta) | **Brief desc**: Add beta interface, IReliableDictionary3, in Reliable Collections. The interface features ranged enumeration based on key comparison. The interface also enables dictionary reads and writes based on sequence numbers. Warning: The interface is not yet suitable for production and is subject to change. |
|**Windows 7.0.457.9590   <br> Ubuntu 7.0.457.1** | **Feature** | Stable reads | **Brief desc**: Add [configurable option to enable stable reads on secondary replicas](https://docs.microsoft.com/azure/service-fabric/service-fabric-reliable-services-configuration#configuration-names-1). Stable reads will restrict secondary replicas to returning values which have been quorum-acked.|

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.0.457.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.0.457.9590 | N/A | https://download.microsoft.com/download/5/e/e/5ee43eba-5c87-4d11-8a7c-bb26fd162b29/MicrosoftServiceFabric.7.0.457.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.0.457.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.0.457.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.0.457.9590.zip |
||Service Fabric Standalone Runtime | 7.0.457.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.0.457.9590/MicrosoftAzureServiceFabric.7.0.457.9590.cab |
|.NET SDK |Windows .NET SDK | 4.0.457 |N/A | https://download.microsoft.com/download/5/e/e/5ee43eba-5c87-4d11-8a7c-bb26fd162b29/MicrosoftServiceFabricSDK.4.0.457.msi |
||Microsoft.ServiceFabric | 7.0.457 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.0.457 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.0.457 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.0.457 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.0.457 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.5 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.5 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 8.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
