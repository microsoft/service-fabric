# Microsoft Azure Service Fabric 7.0 Ninth Refresh Release Notes

This release includes bug fixes as described in this document.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | 7.0.472.1 <br> 7.0.478.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.0.470.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration|4.0.470 <br>7.0.470 <br> 4.0.470 <br> 4.0.470 |
|Java SDK  |Java for Linux SDK  | 1.0.5 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15 <br> 9.0.0 |

## Contents 

Microsoft Azure Service Fabric 7.0 Ninth Refresh Release Notes

* [Service Fabric Runtime](#service-fabric-runtime)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Common Bugs

| Versions | IssueType | Description | Resolution | 
|----------|-----------|-|-|
| **Windows 7.0.478.9590** <br> **Ubuntu 16 7.0.472.1** | **Bug** | Impact to clusters using Open Network Feature | **Brief desc**:  The scope of this issue is limited to the Azure Service Fabric clusters which use [Open Network Feature](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-networking-modes#set-up-open-networking-mode). To improve security and availability, Azure infrastructure will make a breaking change that will affect the supported versions of Service Fabric. This release includes a fix for this issue and is **made available** in all regions. You will have to manually upgrade your clusters if you are affected. To enable cluster support for open network for containerized services, you should have the following properties enabled  in your cluster and application manifest files. The following actions are required to determine if upgrade of Service Fabric runtime is required or not. <br> <br> You have the **IPProviderEnabled setting set to true** in your clusters, you have two options: <br> 1) If you are **NOT using open networking**, then you can disable that setting, setting its value to "false". In the cluster manifest, in the "Hosting" section, look for [IPProviderEnabled and set it to false and upgrade the cluster](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-networking-modes#set-up-open-networking-mode). <br> 2) If you are using "open networking", then you need to upgrade to this version which includes fix for security breaking change.  <br> **Impact**: Nodes won't be able to start and the Service Fabric cluster won't come up if you don't upgrade to this patch release at the time the underlying infrastructure is updated. <br> **Workaround**: N/A <br> **Fix**: Upgrade to this version as soon as possible.


## Repositories and Download Links

The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

|Area |Package | Version | Repository |Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up |7.0.472.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.0.478.9590 | N/A |https://download.microsoft.com/download/0/0/f/00fbca28-0a64-4c9a-a3a3-b11763ee17e5/MicrosoftServiceFabric.7.0.470.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package |7.0.470.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.0.470.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.0.470.9590.zip |
||Service Fabric Standalone Runtime |7.0.470.9590 |N/A |https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.0.470.9590/MicrosoftAzureServiceFabric.7.0.470.9590.cab |
|.NET SDK |Windows .NET SDK | 4.0.470 |N/A |https://download.microsoft.com/download/0/0/f/00fbca28-0a64-4c9a-a3a3-b11763ee17e5/MicrosoftServiceFabricSDK.4.0.470.msi |
||Microsoft.ServiceFabric | 7.0.470 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf |4.0.470|https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*|4.0.470 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.0.470 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.0.470 |N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.5 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.5 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator |1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 9.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
