# Microsoft Azure Service Fabric 7.2 Fourth Refresh Release Notes

This release includes stability fixes as described in this document.

**Due to the holiday season, this update will only be available through manual upgrades. Clusters set to automatic upgrades will not receive this update unless toggled to manual. The Linux release is now also available.**

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | 7.2.447.1 <br> 7.2.447.1804 <br> 7.2.445.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.2.445.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.2.445 <br> 7.2.445 <br> 4.2.445 <br> 4.2.445 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 10.0.0 |

## Content 

Microsoft Azure Service Fabric 7.2 Fourth Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Upcoming Breaking Changes](#breaking-changes)
* [Service Fabric Runtime](#service-fabric-runtime)

## Key Announcements

* .NET 5 apps for Windows on Service Fabric are now supported as a preview. Look out for the GA announcement of .NET 5 apps for Windows on Service Fabric in the coming weeks.
* .NET 5 apps for Linux on Service Fabric will be added in the Service Fabric 8.0 release.
* Windows Server 20H2 is now supported as of the 7.2 CU4 release.

### Update to certificate selection logic

Starting with Service Fabric 7.2 CU4, and for all future SF versions, the logic the runtime uses to select the Cluster certificate has changed. This change applies to both clusters with certificates declared by common name, and clusters with certificates declared by thumbprint when Security/UseSecondaryIfNewer setting is set to true.

Previously, Service Fabric selected among the pool of declared certificates, the certificate that was the farthest living (i.e. the largest NotAfter value). Starting in this CU, Service Fabric will select the most recently issued certificate (i.e. the largest NotBefore value). 

In certain configurations, this upgrade will cause a change to the cluster certificate. In general this change should have no impact on cluster health. However, if clients of Service Fabric expect Service Fabric to present the previous certificate, they may need to be updated to expect the new certificate (or any valid cluster certificate).

Read more [here](https://docs.microsoft.com/azure/service-fabric/cluster-security-certificates)

## Breaking Changes

- Service Fabric 7.2 and higher runtime drops support for .NET core Service Fabric apps running with .NET core 2.2 runtime. .NET core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET core LTS version 3.1.

## Upcoming Breaking Changes

- .NET core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET core apps.  This has no impact on Service Fabric .NET Framework SDK.
- Support for Windows Server 1809 will be discontinued in future Service Fabric releases. We recommend updating your cluster VMs to Windows Server 2019.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.1.458.9590** | **Bug** |Add report code 167 to HealthReport handler |**Brief desc**: Applications which declare endpoint certificates that do not exist on the node will cause a crash in the SF runtime on Linux.<br> **Impact**: SF applications deployed to Linux clusters which declare non-existent endpoint certificates. <br> **Workaround**:  Ensure that certificates declared in application manifests exist on the nodes where the application may be deployed. <br> **Fix**: The health report raised for this scenario was not handled, and entered a code path which crashed the host process.
| **Windows 7.1.458.9590** | **Bug** |Version-less KVRef should be explicitly blocked |**Brief desc**: Only versioned Key Vault objects of object type 'secret' are supported for SF Key Vault References: Read more about [key vault objects.](https://docs.microsoft.com/azure/key-vault/general/about-keys-secrets-certificates#vault-name-and-object-name) <br> **Impact**: Previously version-less objects were allowed to deploy, but have undefined behavior; this fix blocks these deployments. <br> **Workaround**:  N/A <br> **Fix**: Any app using a version-less reference can transition to versioned by upgrading to the object marked "latest" in key-vault explicitly, and using the 'secret' object-type version of that object. You will need to upgrade the app before upgrading the cluster version.
| **Windows 7.1.458.9590** | **Bug** |Deactivation should get blocked if the node has replica with FT in QL state |**Brief desc**: Node Deactivations from MR with intent RemoveNode/RemoveData will now get blocked if the node has a replica which is part of partition which is in Quorum Loss state. <br> **Impact**: Any node deactivations (from MR jobs) or service fabric upgrades will get blocked when these conditions are met.  <br> **Workaround**:  N/A <br> **Fix**: Block these node deactivations when being performed for node which has replica whose partition is in Quorum Loss. This will prevent SF from approving destructive actions like RemoveNode/RemoveData for the service which has not recovered yet.
| **Windows 7.1.458.9590** | **Bug** |Encoding scheme is not correct for authentication with Docker when using REST API's |**Brief desc**: Container Image downloading failed for some passwords when using basic authentication to download docker images.  This was caused due to using an incorrect base64url encoding that Docker REST API's require. <br> **Impact**:  Container image downloading failed with certain passwords that were specified in SF ApplicationManifest.xml files. <br> **Workaround**: Change the password that you use to authenticate to your container registry (most likely Azure Container Registry).  Attempt image download via Service Fabric.  Continue to change password until it works.  Due to encoding issues, some passwords will work, others will not. <br> **Fix**: Corrected the code to use base64url encoding instead of just base64 encoding.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.2.447.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.2.445.9590 | N/A | https://download.microsoft.com/download/4/5/c/45c18017-1e7f-4e2b-87d0-c3d555c54142/MicrosoftServiceFabric.7.2.445.9590.exe |
| Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.2.445.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.2.445.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.2.445.9590.zip |
||Service Fabric Standalone Runtime | 7.2.445.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.2.445.9590/MicrosoftAzureServiceFabric.7.2.445.9590.cab |
|.NET SDK |Windows .NET SDK | 4.2.445 |N/A | https://download.microsoft.com/download/4/5/c/45c18017-1e7f-4e2b-87d0-c3d555c54142/MicrosoftServiceFabricSDK.4.2.445.msi |
||Microsoft.ServiceFabric | 7.2.445 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.2.445 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.2.445 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.2.445 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.2.445 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 10.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
