# Microsoft Azure Service Fabric 7.2 Second Refresh Release Notes

This release includes stability fixes as described in this document. **This update is only rolling out for Windows at this time. Download links will also become available later this week**

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 7.2.431.1 <br> 7.2.431.1804 <br> 7.2.432.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.2.434.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.2.434 <br> 7.2.434 <br> 4.2.434 <br> 4.2.434 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 10.0.0 |

## Content 

Microsoft Azure Service Fabric 7.2 Second Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Upcoming Breaking Changes](#breaking-changes)
* [Service Fabric Runtime](#service-fabric-runtime)

## Key Announcements

* **Fixed stability issues in FabricDNS** - This release fixes stability issues introduced to FabricDNS in the 7.2 release.  Issues seen in this regression include an increased rate of slow/failed DNS requests, and FabricDNS crashing more than usual.

## Upcoming Breaking Changes

- Service Fabric 7.2 and higher runtime drops support for .NET core Service Fabric apps running with .NET core 2.2 runtime. .NET core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET core LTS version 3.1.
- .NET core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET core apps.  This has no impact on Service Fabric .NET Framework SDK.
- .NET 5 apps are currently not supported, support for .NET 5 applications will be added in the Service Fabric 8.0 release.
- We will be deprecating the latest tag for [**OneBox container images**](https://hub.docker.com/_/microsoft-service-fabric-onebox).The new scheme for containers will substitute the current naming scheme ‘latest’ to be replaced with an OS-targeted latest set to explicitly identify the base OS for latest SF releases:
    - mcr.microsoft.com/service-fabric/onebox:u16
    - mcr.microsoft.com/service-fabric/onebox:u18
- The default action of deleting uploaded application package is changed. If the application package is registered successfully, the uploaded application package is deleted by default. Before this default action changes, the uploaded application package was deleted either by calling to [Remove-ServiceFabricApplicationPackage](https://docs.microsoft.com/en-us/powershell/module/servicefabric/remove-servicefabricapplicationpackage) or by using ApplicationPackageCleanupPolicy parameter explicitly with [Register-ServiceFabricApplicationType](https://docs.microsoft.com/en-us/powershell/module/servicefabric/register-servicefabricapplicationtype)
- Service Fabric 7.2 will become the baseline release for future development of image validation. When the feature is activated in future version e.g., Service Fabric 7.3, the cluster is not desired to rolled back down to 7.1 or lower. The version 7.2 is the baseline to which the cluster can downgrade.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.2.432.9590** | **Bug** |FabricDNS regression introduced in 7.2.|**Brief desc**: FabricDNS would not behave as expected after upgrading to 7.2.<br> **Impact**: Issues seen in this regression include an increased rate of slow/failed DNS requests, and FabricDNS crashing more than usual. <br> **Workaround**: Upgrade to 7.2 CU2. <br> **Fix**: This regression has been fixed as part the CU2 release.


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.2.431.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.2.434.9590 | N/A | https://download.microsoft.com/download/4/f/1/4f1d5cd7-4a39-493e-814b-7ccdb671dcd7/MicrosoftServiceFabric.7.2.434.9590.exe |
| Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.2.434.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.2.434.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.2.434.9590.zip |
||Service Fabric Standalone Runtime | 7.2.434.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.2.434.9590/MicrosoftAzureServiceFabric.7.2.434.9590.cab |
|.NET SDK |Windows .NET SDK | 4.2.434 |N/A | https://download.microsoft.com/download/4/f/1/4f1d5cd7-4a39-493e-814b-7ccdb671dcd7/MicrosoftServiceFabricSDK.4.2.434.msi |
||Microsoft.ServiceFabric | 7.2.434 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.2.434 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.2.434 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.2.434 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.2.434 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 10.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
