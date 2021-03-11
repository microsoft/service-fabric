# Microsoft Azure Service Fabric 7.2 Seventh Refresh Release Notes

This release includes stability fixes as described in this document.

**This update will only be available through manual upgrades. Clusters set to automatic upgrades will not receive this update unless toggled to manual.**

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 7.2.476.1 <br> 7.2.476.1804 <br> 7.2.477.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.2.477.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.2.477 <br> 7.2.477 <br> 4.2.477 <br> 4.2.477 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 11.0.0 |

## Content 

Microsoft Azure Service Fabric 7.2 Seventh Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Upcoming Breaking Changes](#breaking-changes)
* [Service Fabric Runtime](#service-fabric-runtime)

## Key Announcements

*  Support for Service Fabric 7.1 will be extended until July 31st, 2021.
*  Key Vault references for Service Fabric applications are now GA on Windows and Linux.
* .NET 5 apps for Windows on Service Fabric are now supported as a preview.
* .NET 5 apps for Linux on Service Fabric will be added in the Service Fabric 8.0 release.

## Breaking Changes

- Service Fabric 7.2 and higher runtime drops support for .NET Core Service Fabric apps running with .NET Core 2.2 runtime. .NET Core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET Core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET Core LTS version 3.1.

## Upcoming Breaking Changes

- .NET Core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET Core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET Core apps.  This has no impact on Service Fabric .NET Framework SDK.
- Support for Windows Server 2016 and Windows 10 Server 1809 will be discontinued in future Service Fabric releases. We recommend updating your cluster VMs to Windows Server 2019.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.2.457.9590** | **Bug** | Encoding scheme is not correct for authentication with Docker when using REST API's |**Brief desc**: Container Image downloading failed for some passwords when using basic authentication to download docker images.  This was caused due to using an incorrect base64url encoding that Docker REST API's require. <br> **Impact**:Container image downloading failed with certain passwords that were specified in SF ApplicationManifest files. <br> **Workaround**:  Change the password that you use to authenticate to your container registry (most likely Azure Container Registry).  Attempt image download via Service Fabric.  Continue to change password until it works.  Due to encoding issues, some passwords will work, others will not.<br> **Fix**:  Corrected the code to use base64url encoding instead of just base64 encoding.
| **Windows 7.2.457.9590** | **Bug** | Improve sfpkg download resilient in network transient error |**Brief desc**:  Downloading SFPKG from web endpoint is subject to network error. Service Fabric runtime changed web client to retry errors by SocketException. <br> **Impact**: Application provisioning with SFPKG could fail by transient error. <br> **Workaround**:  Retry to provision SFPKG. <br> **Fix**: Add retrying exceptions.
| **Windows 7.2.457.9590** | **Bug** | Detect the mismatch of data and image section |**Brief desc**:  Node shutdown in parallel to image download can lead to files filled with zero. <br> **Impact**: Impact in application instance folder is not auto-recovered at next download retry. Impact in Fabric package results in fabric upgrade error. <br> **Workaround**: N/A <br> **Fix**: Downloaded image overwrites files in destination path to reset the content.
| **Windows 7.2.457.9590** | **Bug** | Resolution issues when using fragments in URI |**Brief desc**: Passing an escaped URI fragment without using query strings would cause Reverse Proxy to incorrectly return an error. Reverse Proxy can now handle escaped URI fragments without query strings. <br> **Impact**: Reverse Proxy (FabricApplicationGateway) would incorrectly return FABRIC_E_SERVICE_DOES_NOT_EXIST when using an escaped URI fragment without a query string. <br> **Workaround**: Modify connecting code to not use escaped URI fragments or pass in a dummy query string. <br> **Fix**: Support handling escaped URI fragments correctly in Reverse Proxy, with or without query strings.
| **Windows 7.2.457.9590** | **Bug** | Handle receiving pings when using WebSocket connections |**Brief desc**: Connecting to Reverse Proxy through WebSockets and sending pings would cause Reverse Proxy to crash. Reverse Proxy can now handle receiving WebSocket pings without crashing. <br> **Impact**: Reverse Proxy (FabricApplicationGateway) would crash after receiving a ping when using WebSocket connections. <br> **Workaround**: Modify connecting code to stop sending pings. <br> **Fix**: Support handling ping actions in Reverse Proxy.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.2.476.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.2.477.9590 | N/A | https://download.microsoft.com/download/f/c/3/fc39707f-f67c-4c1b-9274-a055a3eb51b8/MicrosoftServiceFabric.7.2.477.9590.exe |
| Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.2.477.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.2.477.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.2.477.9590.zip |
||Service Fabric Standalone Runtime | 7.2.477.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.2.477.9590/MicrosoftAzureServiceFabric.7.2.477.9590.cab |
|.NET SDK |Windows .NET SDK | 4.2.477 |N/A | https://download.microsoft.com/download/f/c/3/fc39707f-f67c-4c1b-9274-a055a3eb51b8/MicrosoftServiceFabricSDK.4.2.477.msi |
||Microsoft.ServiceFabric | 7.2.477 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.2.477 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.2.477 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.2.477 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.2.477 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
