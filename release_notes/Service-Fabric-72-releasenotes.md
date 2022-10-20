# Microsoft Azure Service Fabric 7.2 Release Notes


This release includes the bug fixes and features described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | NA <br> 7.2.413.9590  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.2.413.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.2.413  <br> 7.2.413 <br> 4.2.413 <br> 4.2.413 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 10.0.0 |

## Contents 
- [Key Announcement](#key-announcements)
- [Breaking Changes](#breaking-changes)
- [Service Fabric Runtime](#service-fabric-runtime)
- [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
- [Reliable Collections](#reliable-collections)
- [Repositories and Download Links](#repositories-and-download-links)

## Key Announcements

- **Preview**: [**Service Fabric managed clusters**](https://techcommunity.microsoft.com/t5/azure-service-fabric/azure-service-fabric-managed-clusters-are-now-in-public-preview/ba-p/1721572) are now in public preview. Service Fabric managed clusters aim to simplify cluster deployment and management by encapsulating the underlying resources that make up a Service Fabric cluster into a single ARM resource. For more details see, [Service Fabric managed cluster overview](https://docs.microsoft.com/azure/service-fabric/overview-managed-cluster).
- **Preview**: [**Supporting stateless services with a number of instances greater than the number of nodes**](https://docs.microsoft.com/azure/service-fabric/service-fabric-cluster-resource-manager-advanced-placement-rules-placement-policies) is now in public preview. A placement policy enables the creation of multiple stateless instances of a partition on a node.
- [**FabricObserver (FO) 3.0**](https://aka.ms/sf/fabricobserver) is now available.
    - You can now run FabricObserver in Linux and Windows clusters.
    - You can now build custom observer plugins. Please see [plugins readme](https://github.com/microsoft/service-fabric-observer/blob/master/Documentation/Plugins.md) and the [sample plugin project](https://github.com/microsoft/service-fabric-observer/tree/master/SampleObserverPlugin) for details and code.
    - You can now change any observer setting via application parameters upgrade. This means you no longer need to redeploy FO to modify specific observer settings. Please see the [sample](https://github.com/microsoft/service-fabric-observer/blob/master/Documentation/Using.md#parameterUpdates).
- [**Support for Ubuntu 18.04 OneBox container images**](https://hub.docker.com/_/microsoft-service-fabric-onebox).
- **Preview**: [**KeyVault Reference for Service Fabric applications supports **ONLY versioned secrets**. Secrets without versions are not supported.**](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-keyvault-references)
- SF SDK requires the latest VS 2019 update 16.7.6 or 16.8 Preview 4 to be able create new .Net Framework stateless/stateful/actors projects. If you do not have the latest VS update, after creating the service project, use package manager to install Microsoft.ServiceFabric.Services (version 4.2.x) for stateful/stateless projects and Microsoft.ServiceFabric.Actors (version 4.2.x) for actor projects from nuget.org.
- **RunToCompletion**: Service Fabric supports concept of run to completion for guest executables. With this update once the replica runs to completion, the cluster resources allocated to this replica will be released.
- [**Resource governance support has been enhanced**](https://docs.microsoft.com/azure/service-fabric/service-fabric-resource-governance): allowing requests and limits specifications for cpu and memory resources.

## Breaking Changes

- Service Fabric 7.2 and higher runtime drops support for .NET core Service Fabric apps running with .NET core 2.2 runtime. .NET core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET core LTS version 3.1.
- .NET core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET core apps.  This has no impact on Service Fabric .NET Framework SDK.
- .NET 5 apps are currently not supported, support for .NET 5 applications will be added in the Service Fabric 8.0 release.
- We will be deprecating the latest tag for [**OneBox container images**](https://hub.docker.com/_/microsoft-service-fabric-onebox).The new scheme for containers will substitute the current naming scheme ‘latest’ to be replaced with an OS-targeted latest set to explicitly identify the base OS for latest SF releases:
    - mcr.microsoft.com/service-fabric/onebox:u16
    - mcr.microsoft.com/service-fabric/onebox:u18
- The default action of deleting uploaded application package is changed. If the application package is registered successfully, the uploaded application package is deleted by default. Before this default action changes, the uploaded application package was deleted either by calling to [Remove-ServiceFabricApplicationPackage](https://docs.microsoft.com/en-us/powershell/module/servicefabric/remove-servicefabricapplicationpackage) or by using ApplicationPackageCleanupPolicy parameter explicitly with [Register-ServiceFabricApplicationType](https://docs.microsoft.com/en-us/powershell/module/servicefabric/register-servicefabricapplicationtype)
- Service Fabric 7.2 will become the baseline release for future development of image validation. When the feature is activated in future version e.g., Service Fabric 7.3, the cluster is not desired to rolled back down to 7.1 or lower. The version 7.2 is the baseline to which the cluster can downgrade.
- Service Fabric 7.2 and higher runtime drops support for .NET Core Service Fabric apps running with .NET Core 2.2 runtime. .NET Core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET Core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET Core LTS version 3.1.<br>
- Guest executable and container applications created or upgraded in SF clusters with runtime versions 7.1+ are incompatible with prior SF runtime versions (e.g. SF 7.0).<br/>
    Following scenarios are impacted:<br/>
    - An application with guest executables or containers is created or upgraded in an SF 7.1+ cluster.<br/>
    The cluster is then downgraded to a previous SF runtime version (e.g. SF 7.0).<br/>
    The application fails to activate.<br/>
    - A cluster upgrade from pre-SF 7.1 version to SF 7.1+ version is in progress.<br/>
    In parallel with the SF runtime upgrade, an application with guest executables or containers is created or upgraded.<br/>
    The SF runtime upgrade starts rolling back (due to any reason) to the pre-SF 7.1 version.<br/>
    The application fails to activate.<br/>

    To avoid issues when upgrading from a pre-SF 7.1 runtime version to an SF 7.1+ runtime version, do not create or upgrade applications with guest executables or containers while the SF runtime upgrade is in progress.<br/>
    - The simplest mitigation, when possible, is to delete and recreate the application in SF 7.0.<br/>
    - The other option is to upgrade the application in SF 7.0 (for example, with a version only change).<br/>

    If the application is stuck in rollback, the rollback has to be first completed before the application can be upgraded again.


## Service Fabric Features

| Versions | IssueType | Description | Resolution | 
|----------|-----------|-|-|
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | Support stateless services with a number of instances greater than the number of nodes | **Brief desc** Public preview feature which enables creation of multiple stateless instances of a partition on a node. Earlier instance count was limited by no. of nodes in the cluster. This feature allows increased density of stateless services without having to manage partitions.  For more details, see [Service Fabric placement policies](https://docs.microsoft.com/azure/service-fabric/service-fabric-cluster-resource-manager-advanced-placement-rules-placement-policies).
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | Validate integrity of files and folders downloaded from the ImageStore | **Brief desc** Service Fabric resolved the race condition with new design of synchronization mechanism in DownloadManager. In addition to the design change, this release includes the required changes for further improvement in the future. The image hash representation may change in the future to improve validation and correction. From this release and onward, the potential new image hash representation is understood for forward compatibility. When the change will be added actually, this release will become the base version to get upgrade to the future version without an impact.
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | Support for using ephemeral OS disks with Service Fabric | **Brief desc** You can now use ephemeral OS disks when on your Virtual Machine Scale Set when deploying a Service Fabric cluster. For more details, see [using ephemeral OS disks](https://docs.microsoft.com/azure/service-fabric/service-fabric-cluster-azure-deployment-preparation#use-ephemeral-os-disks-for-virtual-machine-scale-sets).
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | Resource governance: requests and limits support for cpu and memory | **Brief desc** Prior to version 7.2, SF’s resource governance support for services was based on a model where a single value served both as the request and the limit for a resource. In order to enable more advanced scenarios, this feature enhances SF’s resource governance support for services by allowing separate specifications of requests and limits for cpu and memory resources. Cluster Resource Manager considers the cpu and memory consumption of a service to be equal to the specified request values and uses these values when making placement decisions. The limit values are the actual resource limits applied to a service process or container when it is created on a node. For more details, see [Service Fabric resource governance](https://docs.microsoft.com/azure/service-fabric/service-fabric-resource-governance).
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | Service Fabric Image Store removes local resources used for SMB share| **Brief desc** Service Fabric doesn't use SMB to transfer images between nodes by default, but resources like local accounts and shared folder are created in nodes. Local accounts includes P_FSSUserffffffff and S_FSSUserffffffff. The shared folders are StagingShare_[node name] and StoreShare_[node name]. The change is added to clean up local resources when they are not required. 
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | Image Store CleanupApplicationPackageOnProvisionSuccess default value is changed from false to true 
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | Service Fabric now releases resources allocated to replicas which have ran to completion. | Service Fabric supports notion of ran to completion for guest executables. Once a replica has completed its task, hence ran to completion, Service Fabric will now delete this replica and release cluster resources allocated for it. 
| **Windows 7.1.409.9590  <br> Ubuntu 7.1.410.1** | **Feature** | [Resource governance support has been enhanced](https://docs.microsoft.com/azure/service-fabric/service-fabric-resource-governance) by allowing requests and limits specifications for cpu and memory resources.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.1.458.9590** | **Bug** |CleanFabric.ps1 not cleaning up FabricDataRoot directory|**Brief desc**:  Service Fabric uninstall: CleanFabric.ps1 does not always clean the data root. <br> **Impact**: Runtime installation & node data cleanup operation (CleanFabric.ps1) would at times leave behind the node data root if the machine being cleaned was interrupted during a previous uninstall. <br> **Workaround**:  Set environment variable $FabricDataRoot before running CleanFabric.ps1 which would result in the same behavior as above, or manually delete the directory. <br> **Fix**: An extra optional parameter -FabricDataRoot has been added as an override accepted from the user, ensuring this location is discoverable.
| **Windows 7.1.458.9590** | **Bug** |InstallFabricRuntime: Get-ExecutionPolicy sometimes exits with expected output but with ExitCode: -1073741819|**Brief desc**: SF runtime installation via WebPI/InstallFabricRuntime: Get-ExecutionPolicy sometimes exits with expected output but with ExitCode: -1073741819. <br> **Impact**: InstallFabricRuntime internally executes a powershell script so runs Get-ExecutionPolicy which at times fails with ERROR_ACCESS_DENIED, despite returning the correct mode of operation. <br> **Workaround**:  Run SF runtime installation manually against the exe with parameters: InstallFabricRuntime.exe /accepteula /SkipPSExecutionPolicy. <br> **Fix**: If the expected values are returned, error code is interpreted as non-blocking.
| **Windows 7.1.458.9590** | **Bug** |Image Store CleanupApplicationPackageOnProvisionSuccess default value is changed from false to true |**Brief desc**: Service Fabric applications are copied to ImageStore and then registered for application registration. The copied image is deleted in one of these cases; user’s explicit call to remove image, registering application with automatic cleanup option, or the cluster’s configuration for automatic clean-up at successful registration. <br> **Impact**: A cluster may contain lots of remaining images in ImageStore after application registration. The size of images bloats in ImageStore and could result in low available disk space on the node. <br> **Workaround**:  If the default behavior is not desired, a user can run cluster configuration upgrade to change the default value. <br> **Fix**: The default configuration CleanupApplicationPackageOnProvisionSuccess is set to true.

## Service Fabric Standalone Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.1.458.9590** | **Bug** |Start-ServiceFabricClusterConfigurationUpgrade returns time out exception|**Brief desc**: Start-ServiceFabricClusterConfigurationUpgrade returns time out exception. <br> **Impact**: Client upgrade call would wait until first upgrade failover or timeout instead of exiting and allowing operation to run async once state was persisted to orchestration cycle.  <br> **Workaround**: N/A <br> **Fix**: Client call will now exit successfully once an upgrade is accepted.
| **Windows 7.1.458.9590** | **Bug** |Validation errors in UpdateOrchestrationService do not trigger exceptions or failures during a call to StartClusterConfigurationUpgrade|**Brief desc**:Validation errors in UpdateOrchestrationService do not trigger exceptions or failures during a call to StartClusterConfigurationUpgrade<br> **Impact**: New upgrade requests containing improperly formed settings in the user-provided cluster configuration were in some cases persisted to orchestrated upgrade state, resulting in a cycle that continuously failed manifest validation which wedged cluster upgrade and required manual mitigation. <br> **Workaround**: If an upgrade is found to be pending but does not initiate, submit another dummy cluster configuration upgrade to cancel/bump the existing invalid upgrade.<br> **Fix**: Cluster manifest validation is done up front before committing upgrade state for orchestration cycle.
| **Windows 7.1.458.9590** | **Bug** |Start-ServiceFabricClusterConfigurationUpgrade fails silently|**Brief desc**: Start-ServiceFabricClusterConfigurationUpgrade fails silently. <br> **Impact**: Upgrade requests would in some cases fail silently and fail to return any reason for the internal failure to the user. In these cases sometimes the invalid cluster state may have already been persisted.  <br> **Workaround**: To determine if upgrade was initiated, cluster upgrade state can be queried using Get-ServiceFabricClusterUpgrade and Get-ServiceFabricClusterConfigurationUpgradeStatus. <br> **Fix**: Error handling is now explicit and returns details to the API caller, as well as to the result of query Get-ServiceFabricClusterConfigurationUpgradeStatus.
| **Windows 7.1.458.9590** | **Bug** |Config upgrade rollback along with remove node causes the cluster to end up in an intermediate state|**Brief desc**: Remove node via config upgrade would occasionally leave behind node state for inactive node. <br> **Impact**: An edge case in remove node via config upgrade resulted in skipping the finalization steps of removing node state. <br> **Workaround**: Manual removal of invalid removed node state via Remove-ServiceFabricNodeState. <br> **Fix**:Remove node now progresses with discrete state tracking and nodes are not disabled as part of multi-phase upgrade until the finalization phases. Early termination of FabricUOS should continue node removal and should no longer leave behind invalid node state.
| **Windows 7.1.458.9590** | **Bug** | Pending config upgrade gets kicked off after UOS failover, losing upgrade timing parameters|**Brief desc**: Loss of timing parameters for Start-ServiceFabricClusterConfigurationUpgrade.<br> **Impact**:  Cluster upgrade specifications were lost after failover, which meant that any multi-phase upgrade would be assigned an infinite timeout & default constraints, which may violate the maintenance/fallback loop expected by the user. <br> **Workaround**: Each multi-phase upgrade would need to be monitored by the user and updated by calling Update-ServiceFabricClusterUpgrade. <br> **Fix**: Cluster upgrade specifications are now statefully tracked such that timing characteristics and constraints requested by the user are preserved.
| **Windows 7.1.458.9590** | **Bug** |CreateCluster fails if executed from standalone package under path with space|**Brief desc**: Standalone package scripts could not be run from directory path with space - CreateServiceFabricCluster, AddNode, TestConfiguration, etc. <br> **Impact**: Running package scripts for the first time would fail to extract the DeploymentComponents directory, preventing use of the tools.  <br> **Workaround**:Use base path without spaces. <br> **Fix**:Paths with spaces are now accepted.
| **Windows 7.1.458.9590** | **Bug** |Data loss state restoration doesn't always work|**Brief desc**:  Upgrade Orchestration Service data loss state is not always mitigated by calling config upgrade. <br> **Impact**: A health property reset is required to move FabricUOS (Upgrade Orchestration Service) out of data loss state, and this health update was dropped at times due to a race with garbage collection. <br> **Workaround**:Call Start-ServiceFabricClusterConfigurationUpgrade with dummy upgrade multiple times until data loss health alert is removed. <br> **Fix**: Fabric client was reused and GC.KeepAlive was added in related places to ensure this behaves correctly, fixing the race condition.


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | NA |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.2.413.9590 | N/A | https://download.microsoft.com/download/5/9/d/59d472e7-4c84-4cc5-8622-aeed97895192/MicrosoftServiceFabric.7.2.413.9590.exe |
| Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.2.413.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.2.413.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.2.413.9590.zip |
||Service Fabric Standalone Runtime | 7.2.413.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.2.413.9590/MicrosoftAzureServiceFabric.7.2.413.9590.cab |
|.NET SDK |Windows .NET SDK | 4.2.413 |N/A | https://download.microsoft.com/download/5/9/d/59d472e7-4c84-4cc5-8622-aeed97895192/MicrosoftServiceFabricSDK.4.2.413.msi |
||Microsoft.ServiceFabric | 7.2.413 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.2.413 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.2.413 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.2.413 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.2.413 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 10.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
