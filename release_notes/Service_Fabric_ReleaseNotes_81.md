# Microsoft Azure Service Fabric 8.1  Release Notes

This release includes the bug fixes and features described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises.
This release also include the fixes from 8.0 Third Refresh.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 8.1.320.1 <br>  8.1.320.1 <br> 8.1.316.9590  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 8.1.316.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 5.1.316  <br> 8.1.316 <br> 8.1.316 <br> 8.1.316 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.1.0 |


## Contents 

Microsoft Azure Service Fabric 8.1 Release Notes

* [Key Announcements](#key-announcements)
* [Current And Upcoming Breaking Changes](#Current-And-Upcoming-Breaking-Changes)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
* [Repositories and Download Links](#repositories-and-download-links)


## Key Announcements
* Added support for Auxiliary Replica
* **(Preview)** Added support for .NET 6.0 Service Fabric applications
* Added API support for updating application descriptions
* Added periodic ping between Reconfiguration Agent (RA) and Reconfiguration Agent Proxy (RAP) to detect IPC failure and process stuck
* Added support for liveness and readiness probes for non containerized applications
* Made cluster upgrade for node capacity updates impactless



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
* .NET Core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET Core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .NET runtime 3.* features to support Service Fabric .NET Core apps.  This has no impact on Service Fabric .NET Framework SDK.


## Service Fabric Runtime

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Feature** | Added support for Auxiliary Replica | **Brief desc**: In this release we are introducing support for a new kind of replica. These replicas are called 'Auxiliary Replicas', and their purpose is to enable advanced scenarios where an additional replica is necessary for specialized work required at replicator level that is not currently captured by the existing Roles of Primary, IdleSecondary, ActiveSecondary, None, and Unknown.<br>Auxiliary replicas will not be granted read or write access and Service Fabric will not consider* them as potential targets during Primary elections due to balancing or failover. This opens new opportunities like having a replica that witnesses the replication stream to perform monitoring or auditing, or to act as a temporary cache or transaction log that can be reused by other replicas.<br>One of the applications for auxiliary replicas is assigning them fewer cluster resources since they have fewer duties. This means that in some cases Services can consume fewer resources while maintaining roughly the same levels of reliability, reducing COGS.<br>To use this new feature, the cluster level configuration 'EnableAuxiliaryReplicas' must be set to true and a stateful persisted service would need to be created or updated with the 'AuxiliaryReplicaCount' value, which indicates the number of auxiliary replicas per partition.<br>This is feature is targeted for advanced users/scenarios and require corresponding replicator changes. The default Service Fabric Replicator and Reliable Collections stack does not rely on this, though it may be augmented in future to take advantage of it. Please reach out to Service Fabric team for more details.<br>* Auxiliary replicas can be promoted to primary role under certain scenarios, but this transition is transient, and the replica will not be granted read or write status.<br>**Usage**: New capability in Service Fabric<br>**Impact**: Impacts stateful persisted service using auxiliary replicas.<br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Feature** | **(Preview)** Added support for .NET 6.0 Service Fabric applications | **Brief desc**: Service Fabric version 8.1 introduces programming model support for .NET 6.0 Service Fabric applications.<br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Feature** | Added API support for updating application descriptions | **Brief desc**: Service Fabric version 8.1 introduces REST API support for updating Service Fabric applications. Customers can specify an application name and denote application parameters to be updated. These parameters include the maximum nodes, minimum nodes, application metrics and whether application capacities should be removed.<br>Corresponding PowerShell API is documented here: https://docs.microsoft.com/en-us/powershell/module/servicefabric/update-servicefabricapplication?view=azureservicefabricps.<br>**Usage**: Added UpdateApplication REST API<br>**Impact**: Customer is able to update application with REST API.<br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Feature** | Added periodic ping between RA and RAP to detect IPC failure and stuck process | **Brief desc**: Added periodic ping between Reconfiguration Agent (RA) and Reconfiguration Agent Proxy (RAP) to detect IPC failure and stuck process<br>**Usage**: On every node, the Service Fabric agent on the node will periodically exchange a heartbeat message with the customer process, to ensure the IPC is working and the customer process is not frozen. When such issues have been detected (not a single heartbeat message has been successfully exchanged in 15 min by default), the customer process will be terminated to mitigate the issue.<br>**Impact**: Add periodic ping between RA and RAP to detect IPC failure and stuck process<br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Feature** | Added support probes for non containerized applications | **Brief desc**: Support for liveness and readiness probe for non containerized applications<br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Feature** | Made cluster upgrade for node capacity updates impactless | **Brief desc**: Service Fabric version 8.1 introduces support for dynamic upgrade of node capacity and node property configs, with no disruption to running workloads.<br>**Impact**: Customer is able to update node capacities and node properties via dynamic config upgrade.<br> | 


## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Bug** | SF DNS - Use fixed pool of sockets for recursive queries | **Brief desc**: Recursive queries in SF DNS now re-use sockets from a fixed port range. In non-standard deployment scenarios (i.e. not using Azure VMs with the default DNS server - 168.63.129.16), the port range must be opened up in the firewall. The start address and size can be configured through the following options under DnsService:<br>* **ForwarderPoolStartPort**: The start address for the forwarding pool that is used for recursive queries. The default value is 16700.<br>* **ForwarderPoolSize**: The number of forwarders in the forwarding pool. The default value is 20. <br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Bug** | SF DNS - Improve retry capabilities when calling SF APIs to resolve names | **Brief desc**: This change retires the RetryTransientFabricErrors config option under DnsService and replaces them with two new options.<br>* **TransientErrorMaxRetryCount**: Controls the number of times SF DNS will retry when a transient error occurs while calling SF APIs (e.g. when retrieving names and endpoints). The default value is 3.<br>* **TransientErrorRetryIntervalInMillis**: Sets the delay in milliseconds between retries for when SF DNS calls SF APIs. The default value is 0. <br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Bug** | FM resolution failures should be retried instead of exiting ISMgr loop rightaway | **Brief desc**: For the CrossAZ Nodetype, sometimes IS's can get stuck in the error state on VMSS deletion/deallocation. This fix ensures that the IS's are correctly managed when they are added/deleted/updated as required.<br>**Impact**: CrossAZ VMSS Deletion/Deallocation can leave IS's in error state, as FM resolution can break intermittently.<br>**Fix**: Gracefully handle FM resolution failures.<br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Bug** | Expose the PLB Decision trace as SF diagnostic event | **Brief desc**: Service Fabric version 8.1 introduces possibility to expose PLB Decision trace as SF diagnostic event in case move or new placement happened. In order to enable the feature, TraceCRMReasons and DecisionOperationalTracingEnabled must be turned on.<br> |
| **Windows - 8.1.316.9590<br>Ubuntu 16 - 8.1.320.1<br>Ubuntu 18 - 8.1.320.1** | **Bug** | Net 6.0 Service fabric app using remoting throws "CLR detected invalid program" error | **Brief desc**: Remoting V2 service communication fails for Net6.0 apps with "CLR detected invalid program" error<br>**Impact**: Add preview support for Service Fabric services targeting Net6.0<br>**Fix**: Fixed the IL code generation logic to produce Net6.0 compatible code<br> |


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 8.1.320.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 8.1.316.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.8.1.316.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 8.1.316.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/8.1.316.9590/Microsoft.Azure.ServiceFabric.WindowsServer.8.1.316.9590.zip |
||Service Fabric Standalone Runtime | 8.1.316.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/8.1.316.9590/MicrosoftAzureServiceFabric.8.1.316.9590.cab |
|.NET SDK |Windows .NET SDK | 5.1.316 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.5.1.316.msi |
||Microsoft.ServiceFabric | 8.1.316 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 8.1.316 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.1.316 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 8.1.316 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.1.316 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.1.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
