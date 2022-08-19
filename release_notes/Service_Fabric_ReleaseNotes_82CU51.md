# Microsoft Azure Service Fabric 8.2 Cumulative Update 5.1 Release Notes

This release includes bug fixes as described in this documents.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Ubuntu 20 | 8.2.1483.1 <br> 8.2.1483.1 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |


## Contents 

Microsoft Azure Service Fabric 9.0 Cumulative Update 2.0 Release Notes

* [Current Breaking Changes](#current-breaking-changes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)


##Current Breaking Changes
Customers running Azure Service Fabric Linux Clusters will not be able to scale out or reimage (manual or auto via VMSS OS Image Update) nodes  or create new Linux clusters without upgrading to this version of Service Fabric(8.2.1483.1). Also, if your application requires JDK, then you will need to install a JDK of your choice since SF will not install the JDK as part of the SF installation.
 The following versions will be deprecated starting August 19 and will not be available on Linux for new cluster creation or node reimage/scale out operations: 
	* 9.0 CU2/CU1/RTO
	* 8.2 CU4/CU3/CU2.1/CU2/CU1/RTO
Details: Service Fabric Runtime installed a dependency (Azul JDK), which is now deprecated, and unavailable, as announced in dev [blog](https://nam06.safelinks.protection.outlook.com/?url=https%3A%2F%2Fdevblogs.microsoft.com%2Fjava%2Fend-of-updates-support-and-availability-of-zulu-for-azure%2F&data=05%7C01%7CDivya.Cherkuri%40microsoft.com%7Cf02580b7e1304da015b508da7982378c%7C72f988bf86f141af91ab2d7cd011db47%7C1%7C0%7C637955897433879958%7CUnknown%7CTWFpbGZsb3d8eyJWIjoiMC4wLjAwMDAiLCJQIjoiV2luMzIiLCJBTiI6Ik1haWwiLCJXVCI6Mn0%3D%7C3000%7C%7C%7C&sdata=WmkPbF100N60IK%2BgPJ3lXdfvQR0JO%2Fy3pqzdJWRcfnI%3D&reserved=0). Since, on Linux, creation of new clusters or scale-out/reimage of nodes required all the dependencies to be installed, these workflows broke when the dependency became unavailable.
Starting with this release, Service Fabric Runtime will stop installing Azul JDK dependency which will ensure that customer can scale out, reimage, or create new Linux clusters.  If your application needs JDK, please utilize an alternate mechanism to provision a JDK.  To get the latest SF Runtime releases with this change, follow the upgrade documentation and install a JDK of your choice. To get the latest SF Runtime releases with this change, follow the [upgrade documentation](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-tutorial-upgrade-cluster) and install a JDK of your choice.
For more information see:[Breaking change for Azure Service Fabric Linux customers]()

## Retirement and Deprecation Path Callouts
* Service Fabric ASP.NET Core packages built against ASP.NET Core 1.0 binaries are out of support. Starting Service Fabric 8.2, we will be building Service Fabric ASP.NET Core packages against .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, .NET Framework 4.6.1. As a result, they can be used only to build services targeting .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, >=.NET Framework 4.6.1 respectively.
For .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, Service Fabric ASP.NET Core will be taking dependency on Microsoft.AspNetCore.App shared framework; whereas, for NetFx target frameworks >=.NET Framework 4.6.1, Service Fabric ASP.NET Core will be taking dependency on ASP.NET Core 2.1 packages.
The package Microsoft.ServiceFabric.AspNetCore.WebListener will no longer be shipped, Microsoft.ServiceFabric.AspNetCore.HttpSys package should be used instead.
* .NET 5.0 runtime has reached end-of-life on May 8, 2022.Service Fabric releases after this date will drop support for Service Fabric applications running with .NET 5.0 runtime.Current applications running on .NET 5.0 runtime will continue to work, but requests for investigations or request for changes will no longer be entertained. Please migrate to using .NET 6.0 version instead.
* Ubuntu 16.04 LTS reached its 5-year end-of-life window on April 30, 2021.Service Fabric runtime has dropped support for 16.04 LTS, and we recommend moving your clusters and applications to Ubuntu 18.04 LTS or 20.04 LTS.Current applications running on it will continue to work, but requests to investigate issues w.r.t this OS or requests for change will no longer be entertained.We have stopped producing Service Fabric builds for U16.04 so customers will no longer get SF related patches. Please migrate to Ubuntu 18.04 or 20.04 instead.
* Service Fabric runtime will soon stop supporting BinaryFormatter based remoting exception serialization by default and move to using Data Contract Serilization (DCS) based remoting exception serialization by default.Current applications using it will continue to work as-is, but Service Fabric strongly recommends moving to using Data Contract Serilization (DCS) based remoting exception instead.
* Service Fabric runtime will soon be archiving and removing Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older from the package Download Center.Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors. 

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release.
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 8.2.1483.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| N/A | N/A | N/A |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | N/A |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/9.0.1048.9590/Microsoft.Azure.ServiceFabric.WindowsServer.9.0.1048.9590.zip |
||Service Fabric Standalone Runtime | N/A |N/A | N/A |
|.NET SDK |Windows .NET SDK | N/A |N/A | N/A |
||Microsoft.ServiceFabric | N/A |N/A |N/A |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | N/A |N/A |N/A |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.0.516 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 9.0.1048 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.0.516 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.1 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |
