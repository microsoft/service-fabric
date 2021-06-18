# Microsoft Azure Service Fabric 8.0 Second Refresh Release Notes

This release includes bug fixes as described in this document.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | N/A <br>  N/A <br> 8.0.521.9590  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 8.0.521.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 5.0.521  <br> 8.0.521 <br> 8.0.521 <br> 8.0.521 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.0 |


## Contents 

Microsoft Azure Service Fabric 8.0 Second Refresh Release Notes

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
* .NET Core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET Core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .NET runtime 3.* features to support Service Fabric .NET Core apps.  This has no impact on Service Fabric .NET Framework SDK.


## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.0.521.9590<br>Ubuntu 16 - 8.0.517.1<br>Ubuntu 18 - 8.0.517.1804** | **Bug** | Fix file leak in SF Runtime in Linux | **Brief desc**: A bug caused FD leak in Linux when installing new application packages.<br>**Impact**: FD leak and cause cannot open file.<br>**Change**: The bug of FD leaking of ImageStore. | 
| **Windows - 8.0.521.9590<br>Ubuntu 16 - 8.0.517.1<br>Ubuntu 18 - 8.0.517.1804** | **Bug** | Enhance BRS service to support containerized stateful service | **Brief desc**: Enhanced Backup and Restore service (BRS) to support containerized stateful service: BackupCopier (responsible for uploading/downloading backups) runs on host while the backups are taken inside the container. Since host and container don't share the same storage namespace, BackupCopier used to fail to upload backups taken in the service context with file not found exception. In this release, we have added functionality to ensure that backup taken in service context inside container is accessible by BackupCopier running on host. This enables BRS to function on containerized stateful services.<br>**Impact**: BackupRestoreService doesn't support backup/restore operations for the containerized services. BackupCopier (responsible for uploading/downloading backups) runs on host while the backups are taken inside the container. Since host and container don't share the same storage namespace, BackupCopier used to fail to upload backups taken in the service context with file not found exception.<br>**Change**: We have added functionality to ensure that backup taken in service context inside container is accessible by BackupCopier running on host. This enables BRS to function on containerized stateful services. | 
| **Windows - 8.0.521.9590<br>Ubuntu 16 - 8.0.517.1<br>Ubuntu 18 - 8.0.517.1804** | **Bug** | Support compressed package and download package for local dev cluster | **Brief desc**: A local SF cluster that is created from Service Fabric SDK can't deploy compressed package, when multiple replica from same service package are activated on the same node.<br>**Impact**: A developer can't test compressed package uploading with local cluster.<br>**Change**: The bug of uncompressing zip file twice has been fixed. | 
| **Windows - 8.0.521.9590<br>Ubuntu 16 - 8.0.517.1<br>Ubuntu 18 - 8.0.517.1804** | **Bug** | Ensure FileStoreService replica succeeds at replication even when file size can't be set | **Brief desc**: ImageStoreService replica copies images between replicas. The replication didn't fail incorrectly at preserving the space for a file on disk, while the disk doesn't have enough available space to store it.<br>**Impact**: ImageStoreService, running in a disk with no available space, commits the replicated file successfully. The subsequent download of the image returns files in size of zero.<br>**Change**: The bug has been fixed to return error, if the disk for replicated file can't be preallocated. | 
| **Windows - 8.0.521.9590<br>Ubuntu 16 - 8.0.517.1<br>Ubuntu 18 - 8.0.517.1804** | **Bug** | Support reusing same version and changing zip to non-zip package | **Brief desc**: A user may register a version of service package in compressed format and create instances of the service. Then, the application has been deleted and the package has been unregistered. The user could reuse the same version to upload different set of binaries for the same service package name, for example in development environment. The second upload, however, might change to upload in non-compressed format. In this specific sequence of events, SF cluster can't activate the service package by internal error.<br>**Impact**: The internal management of image path has a bug that can't activate service replica in the mentioned situation.<br>**Change**: The internal management image cache path has been fixed and the second upload's change from compressed to non-compressed image is detected successfully. | 


## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | N/A |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 8.0.521.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.8.0.521.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 8.0.521.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/8.0.521.9590/Microsoft.Azure.ServiceFabric.WindowsServer.8.0.521.9590.zip |
||Service Fabric Standalone Runtime | 8.0.521.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/8.0.521.9590/MicrosoftAzureServiceFabric.8.0.521.9590.cab |
|.NET SDK |Windows .NET SDK | 5.0.521 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.5.0.521.msi |
||Microsoft.ServiceFabric | 8.0.521 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 8.0.521 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.0.521 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 8.0.521 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.0.521 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
