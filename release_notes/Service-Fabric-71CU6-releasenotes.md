# Microsoft Azure Service Fabric 7.1 Fifth Refresh Release Notes

This release includes bug fixes and features as described in this document.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 7.1.455.1 <br> 7.1.455.1804 <br> 7.1.459.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.1.458.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.1.458 <br> 7.1.458 <br> 4.1.458 <br> 4.1.458 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 10.0.0 |

## Content 

Microsoft Azure Service Fabric 7.1 Fifth Refresh Release Notes

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.1.458.9590** | **Bug** |XmlLiteReader.Linux.cpp leaks memory on close|**Brief desc**: Fabric would leak memory when processing XML manifest.  <br> **Impact**: Large amount of manifest processing would result in memory leaking over a period of time.  <br> **Workaround**: N/A <br> **Fix**: The leak has been fixed.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.1.455.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.1.458.9590 | N/A | https://download.microsoft.com/download/e/3/c/e3ccf2e1-2c80-48b3-9a8d-ce0dbd67bb77/MicrosoftServiceFabric.7.1.458.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package |7.1.458.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.1.458.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.1.458.9590.zip |
||Service Fabric Standalone Runtime |7.1.458.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.1.458.9590/MicrosoftAzureServiceFabric.7.1.458.9590.cab |
|.NET SDK |Windows .NET SDK |4.1.458 |N/A | https://download.microsoft.com/download/e/3/c/e3ccf2e1-2c80-48b3-9a8d-ce0dbd67bb77/MicrosoftServiceFabricSDK.4.1.458.msi |
||Microsoft.ServiceFabric |7.1.458 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf |4.1.458|https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*|4.1.458 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal |4.1.458 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions |4.1.458 |N/A |https://www.nuget.org |
|Java SDK |Java SDK |1.0.6 |N/A | https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse |2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator |1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator |1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator |1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators |1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI |10.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric |0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |

