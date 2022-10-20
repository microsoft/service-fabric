# Microsoft Azure Service Fabric 8.0  Release Notes

This release includes the bug fixes and features described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 8.0.513.1 <br>  8.0.513.1804 <br> 8.0.514.9590  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 8.0.514.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 5.0.514  <br> 8.0.514 <br> 8.0.514 <br> 8.0.514 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.0 |


## Contents 

Microsoft Azure Service Fabric 8.0 Release Notes

* [Key Announcements](#key-announcements)
* [Current And Upcoming Breaking Changes](#Current-And-Upcoming-Breaking-Changes)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
* [Repositories and Download Links](#repositories-and-download-links)


## Key Announcements
* Support for .NET 5 for Windows is now generally available
* Stateless NodeTypes are now generally available
* Ability to move stateless service instances
* Ability to add parameterized DefaultLoad in the application manifest
* For singleton replica upgrades - ability to have some of the cluster level settings to be defined at an application level
* Ability for smart placement based on node tags
* Ability to define percentage threshold of unhealthy nodes that influence cluster health
* Ability to query top loaded services
* Ability to add a new interval for new error codes 
* Ability to mark service instance as completed
* Support for wave-based deployment model for automatic upgrades
* Added readiness probe for containerized applications
* Enable UseSeparateSecondaryMoveCost to true by default
* Fixed StateManager to release the reference as soon as safe to release
* Block Central Secret Service removal while storing user secrets


## Current Breaking Changes
* Service Fabric 7.2 and higher runtime drops support for .NET Core Service Fabric apps running with .NET Core 2.2 runtime. .NET Core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET Core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET Core LTS version 3.1.<br>
* Guest executable and container applications created or upgraded in SF clusters with runtime versions 7.1+ are incompatible with prior SF runtime versions (e.g. SF 7.0).<br/>
    Following scenarios are impacted:<br/>
    * An application with guest executables or containers is created or upgraded in an SF 7.1+ cluster.<br/>
    The cluster is then downgraded to a previous SF runtime version (e.g. SF 7.0).<br/>
    The application fails to activate.<br/>
    * A cluster upgrade from pre-SF 7.1 version to SF 7.1+ version is in progress.<br/>
    In parallel with the SF runtime upgrade, an application with guest executables or containers is created or upgraded.<br/>
    The SF runtime upgrade starts rolling back (due to any reason) to the pre-SF 7.1 version.<br/>
    The application fails to activate.<br/>

    To avoid issues when upgrading from a pre-SF 7.1 runtime version to an SF 7.1+ runtime version, do not create or upgrade applications with guest executables or containers while the SF runtime upgrade is in progress.<br/>
    * The simplest mitigation, when possible, is to delete and recreate the application in SF 7.0.<br/>
    * The other option is to upgrade the application in SF 7.0 (for example, with a version only change).<br/>

    If the application is stuck in rollback, the rollback has to be first completed before the application can be upgraded again.


## Upcoming Breaking Changes
* .NET Core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET Core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET Core apps.  This has no impact on Service Fabric .NET Framework SDK.
* Support for Windows Server 2016 **Version 1809** will be discontinued in future Service Fabric releases. We recommend updating your cluster VMs to Windows Server 2019.
* Support for Windows 10 **Version 1809** will be discontinued in future Service Fabric releases. We recommend updating your cluster VMs to a supported version of Windows 10.


## Service Fabric Runtime

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Enable support for .NET 5 for Windows | **Brief desc**: Service Fabric 8.0 announce the general availability of the .NET 5 Windows Applications.<br>This feature includes full support in production for Reliable Services (stateless or stateful) and Reliable Actors. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Enable support for Stateless NodeTypes | **Brief desc**: Stateless node types are a new property which will only place stateless services on the specific node type. This enables several benefits for stateless workloads:<br/><li> Faster scale out operations.<br/><li> Support for scale sets with greater than 100 nodes. (Support for > 100 nodes in a scale sets to simplify meeting demand)<br/><li> Support for VMSS Automatic OS upgrades on bronze durability to further simplify OS patching.<br/>**Usage**: To try out stateless node types, see the documentation for <a href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-stateless-node-types">Stateless Node Types</a> and the <a href="https://github.com/Azure-Samples/service-fabric-cluster-templates/tree/master/10-VM-2-NodeTypes-Windows-Stateless-Secure">sample templates</a>. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Ability to move stateless service instances | **Brief desc**: Service Fabric 8.0 introduces the support to move instances of stateless services. This functionality is similar to the existing functionality of moving replicas of stateful services.<br>**Usage**: User can specify stateless service name, partition id, name of the node with the instance to be moved, name of the node to which the instance should be moved and whether Cluster Resource Manager constraints should be ignored.<br>This functionality can be achieved through PowerShell, REST or fabric client C# API.<br>**Impact**: Ability to manually move instances of stateless services.<br>Fault Injection and Cluster Analysis Service have also been extended with a new type of fault: Move an instance. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Ability to add parameterized DefaultLoad in the application manifest | **Brief desc**: Service Fabric 8.0 introduces the ability to parametrize default loads both in the application and in the service manifests.<br>**Usage**: User can now specify application parameter name inside square brackets for attributes DefaultLoad, PrimaryDefaultLoad and SecondaryDefaultLoad for LoadMetric element. &#x3C;LoadMetrics&#x3E;&#x3C;LoadMetric Name="MetricA" PrimaryDefaultLoad="[AppParam1]" SecondaryDefaultLoad="[AppParam2]" /&#x3E;&#x3C;/LoadMetrics&#x3E;<br>**Impact**: Ability to parameterize default loads.<br>This also required changing LoadMetricType field from long to string data type.<br>**Workaround**: Run application upgrade to update default load in application manifest | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | For singleton replica upgrades - ability to have some of the cluster level settings to be defined at an application level | **Brief desc**: The service Description for stateful and stateless services has been extended in Service Fabric 8.0.<br>**Usage**: Stateless services have InstanceLifecycleDescription which contains RestoreReplicaLocationAfterUpgrade parameter.<br>Stateful services have ReplicaLifecycleDescription which contains IsSingletonReplicaMoveAllowedDuringUpgrade and RestoreReplicaLocationAfterUpgrade parameter.<br>**Workaround**: Without this change these parameters can be set at a cluster level only.<br>With this change the cluster can be organized more granularly. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Ability for smart placement based on node tags | **Brief desc**: Service Fabric 8.0 enables support for tag definition and allows tagging a node during placement. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Ability to define percentage threshold of unhealthy nodes that influence cluster health | **Brief desc**: Add option to ClusterHealthPolicy to evaluate cluster health with node type health policies. <br>**Usage**: Cluster manifest will have new object under HealthManger/ClusterHealthPolicy section to specify node type specific policies.<br>This is an opt in feature. Configuration entry HealthManager/EnableNodeTypeHealthEvaluation must be enabled for this feature to work.<br>**Impact**: Allow health evaluations to have a new metric to evaluate by - max percent unhealthy nodes per node type.<br>Queries and APIs will have new field mapping node type name to max percent unhealthy tolerated.<br>This feature is not enabled while upgrading from a version with this feature to one without this feature.<br>If no policy is specified for a node type, then that node type is not evaluated for anything (that is the equivalent to the default being 100% unhealthy allowance), and the status will depend on other node health policies, such as max percent unhealthy nodes.<br>This feature will not have any impact on other node based health policies, and specifying a node type health policy does not take the node out of the global pool for max percent unhealthy nodes.<br>**Documentation**: https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-health-introduction | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Ability to query top loaded services | **Brief desc**: Service Fabric 8.0 introduces the support to query top/least loaded partitions based on metric name. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Ability to add a new interval for new error codes  | **Brief desc**: Service Fabric 8.0 adds an additional error range for the possible errors returned from its API's.<br>Previously the range was in `FACILITY_WIN32 (0x7)` between the error range from 0x7100 to 0x7499.<br>This range has now been extended to a Service Fabric specific facility `FACILITY_SERVICE_FABRIC (0x7b0)` for the entire range from 0x0000 to 0xffff.<br>**Usage**: When consuming errors directly, augment handling logic to now include errors in the new range as well.<br>**Impact**: Existing errors remain unchanged and will not require any new logic to handle it. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Capability to mark service instance as completed | **Brief desc**: New ReportCompletion API added to IStatelessServicePartition interface.<br>This API allows stateless instances to mark themselves as completed.<br>**Impact**: Adds new capability for the stateless services.<br>Once the API is called, Service Fabric will close the stateless instance and will not create replacement instance in the cluster. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Support for wave based deployment model for automatic upgrades | **Brief desc**: With automatic upgrade mode, you have the option to enable your cluster for wave deployment. With wave deployment, you can create a pipeline for upgrading your test, stage, and production clusters in sequence, separated by built-in 'bake time' to validate upcoming Service Fabric versions before your production clusters are updated.<br>**Usage**: To enable wave deployment for automatic upgrade, first determine which wave to assign your cluster:<br/>**Wave 0** ('Wave0'): Clusters are updated as soon as a new Service Fabric build is released. Intended for test/dev clusters.<br/>**Wave 1** ('Wave1'): Clusters are updated one week (seven days) after a new build is released. Intended for pre-prod/staging clusters.<br/>**Wave 2** ('Wave2'): Clusters are updated two weeks (14 days) after a new build is released. Intended for production clusters.<br/>Then, simply add an 'upgradeWave' property to your cluster resource template with one of the wave values listed above.<br/>Ensure your cluster resource API version is '2020-12-01-preview' or later. | 
| **Windows - 8.0.512.9590<br>Ubuntu 16 - 8.0.511.1<br>Ubuntu 18 - 8.0.511.1804** | **Feature** | Added readiness probe for containerized applications | **Brief desc**: Service Fabric 8.0 announce the support for readiness probe for containerized applications<br>**Impact**: Readiness Probes are updated when a container is ready to start accepting traffic. This makes the service discoverable when the replica endpoint is published.<br>If the probe reports failure, Service Fabric will remove the replica endpoint. | 



## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.0.514.9590<br>Ubuntu 16 - 8.0.513.1<br>Ubuntu 18 - 8.0.513.1804** | **Bug** | Enable UseSeparateSecondaryMoveCost to true by default | **Brief desc**: With Service Fabric 8.0, config UseSeparateSecondaryMoveCost is enabled by default. <br>**Impact**: This fix enables the ability to distinguish move cost between different secondary replicas (readable/non-readable etc.).<br>Reported move cost for secondary on one node will take effect only on that secondary, and no effect on secondary replicas on other nodes. | 
| **Windows - 8.0.514.9590<br>Ubuntu 16 - 8.0.513.1<br>Ubuntu 18 - 8.0.513.1804** | **Bug** | Fixed StateManager to release the reference as soon as safe to release | **Brief desc**: StateManager was holding on to references of Reliable Collections long after they were deleted if machine memory usage was lower than 50%.<br>With Service Fabric 8.0, the StateManager was fixed to release the reference as soon as it is safe to release.<br>**Impact**: This fix Impacts users of Reliable Collections on Windows with cluster version >= 7.1.<br>The impact should be low since the references will be removed once machine memory usage exceeds 50%. | 
| **Windows - 8.0.514.9590<br>Ubuntu 16 - 8.0.513.1<br>Ubuntu 18 - 8.0.513.1804** | **Bug** | Block Central Secret Service removal while storing user secrets | **Brief desc**: To prevent accidental data loss, Central Secret Service now requires two cluster configuration upgrades to remove.<br>**Workaround**: 'IsEnabled' is still allowed, but is now deprecated. To remove CSS, the first upgrade will need to move from IsEnabled=true to DeployedState=Removing.<br>**Documentation**: https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-application-secret-store | 



## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 8.0.513.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 8.0.514.9590 | N/A | https://download.microsoft.com/download/4/9/5/49541d73-6f85-4755-b41b-6b71528f2cfd/MicrosoftServiceFabric.8.0.514.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 8.0.514.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/8.0.514.9590/Microsoft.Azure.ServiceFabric.WindowsServer.8.0.514.9590.zip |
||Service Fabric Standalone Runtime | 8.0.514.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/8.0.514.9590/MicrosoftAzureServiceFabric.8.0.514.9590.cab |
|.NET SDK |Windows .NET SDK | 5.0.514 |N/A | https://download.microsoft.com/download/4/9/5/49541d73-6f85-4755-b41b-6b71528f2cfd/MicrosoftServiceFabricSDK.5.0.514.msi |
||Microsoft.ServiceFabric | 8.0.514 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 8.0.514 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.0.514 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 8.0.514 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.0.514 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
