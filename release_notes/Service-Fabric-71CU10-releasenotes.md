# Microsoft Azure Service Fabric 7.1 Tenth Refresh Release Notes

This release includes stability, performance, and bug fixes.

**This update will only be available on Windows, and through manual upgrades. Clusters set to automatic upgrades will not receive this update unless toggled to manual.**

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Windows |  7.1.510.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.1.510.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.1.510 <br> 7.1.510 <br> 4.1.510 <br> 4.1.510
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 10.0.0 |

## Content

Microsoft Azure Service Fabric 7.1 Tenth Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Upcoming Breaking Changes](#breaking-changes)
* [Service Fabric Runtime](#service-fabric-runtime)

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment:
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Windows Developer Set-up| 7.1.510.9590 | N/A | https://download.microsoft.com/download/4/e/f/4efd3f5c-17a5-470d-99a8-ca8d8fb0a9b6/MicrosoftServiceFabric.7.1.510.9590.exe |
| Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.1.510.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.1.510.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.1.510.9590.zip |
||Service Fabric Standalone Runtime | 7.1.510.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.1.510.9590/MicrosoftAzureServiceFabric.7.1.510.9590.cab |
|.NET SDK |Windows .NET SDK | 4.1.510 |N/A | https://download.microsoft.com/download/4/e/f/4efd3f5c-17a5-470d-99a8-ca8d8fb0a9b6/MicrosoftServiceFabricSDK.4.1.510.msi |
||Microsoft.ServiceFabric | 7.1.510 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.1.510 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.1.510 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.1.510 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.1.510 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 10.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
