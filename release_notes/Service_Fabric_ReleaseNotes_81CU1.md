# Microsoft Azure Service Fabric 8.1 First Refresh Release Notes

This release includes the bug fixes described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 8.1.323.1 <br> 8.1.323.1 <br> 8.1.321.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 8.1.321.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 5.1.321 <br> 8.1.321 <br> 8.1.321 <br> 8.1.321 |
|Java SDK |Java for Linux SDK | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module <br> SFCTL | 0.3.15 <br> 11.1.0 |


## Contents 

Microsoft Azure Service Fabric 8.1 First Refresh Release Notes

* [Current And Upcoming Breaking Changes](#Current-And-Upcoming-Breaking-Changes)
* [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
* [Repositories and Download Links](#repositories-and-download-links)


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
* .NET Core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET Core 2.1 runtime. This includes the following release versions and beyond - 8.0 CU4 and above, 8.1 CU2 and above and 8.2 RTO release and above. Current apps running on .NET Core 2.1 runtime will continue to work, but requests for investigations or request for changes will no longer be entertained. Service Fabric .NET SDK will take a dependency on .NET runtime 3.* features to support Service Fabric .NET Core apps. This has no impact on Service Fabric .NET Framework SDK.


## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.1.321.9590<br>Ubuntu 16 - 8.1.323.1<br>Ubuntu 18 - 8.1.323.1** | **Bug** | Upgrade of stateful and stateless services fail with Service DNS name in use error | **Brief desc**: Azure Service Fabric Service Update fails with DnsNameInUse if ServiceDnsName is set. <br> **Impact**: Updating Service Fabric service via ARM is being blocked when ServiceDnsName is unchanged. This is blocking service update flows for some users. This is resulting from a new client parameterization that exposed an underlying behavior that blocked no-ops. <br> **Fix** NamingService is now handling the ServiceDnsName parameter as a no-op when it is unchanged. <br> **Workaround** Update service using Service Fabric PowerShell cmdlet Update-ServiceFabricService, omitting the ServiceDnsName parameter if it is unchanged. |
| **Windows - 8.1.321.9590<br>Ubuntu 16 - 8.1.323.1<br>Ubuntu 18 - 8.1.323.1** | **Bug** | NamingStoreService.RecoverPrimaryAsyncOperation keeps retrying rejected operations for codes such as DnsNameInUse | **Brief desc**: NamingService would perpetually retry a subset of rejected operations, which risks congesting the job pipeline. <br> **Impact**: Naming StoreService may perpetually hold onto rejected operations and requeue them to the job queue. This happened for rejected operations that tried to override dns name of another service ErrorCodeValue::DnsNameInUse. <br> **Fix** An extra filter has been added to discard rejected operations for known error codes. |


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 8.1.323.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 8.1.321.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.8.1.321.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 8.1.321.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/8.1.321.9590/Microsoft.Azure.ServiceFabric.WindowsServer.8.1.321.9590.zip |
||Service Fabric Standalone Runtime | 8.1.321.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/8.1.321.9590/MicrosoftAzureServiceFabric.8.1.321.9590.cab |
|.NET SDK |Windows .NET SDK | 5.1.321 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.5.1.321.msi |
||Microsoft.ServiceFabric | 8.1.321 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 8.1.321 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.1.321 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 8.1.321 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.1.321 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.1.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15 |
