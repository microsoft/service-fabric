# Microsoft Azure Service Fabric 7.1 Second Refresh Release Notes

This release includes bug fixes and features as described in this document.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | 7.1.428.1 <br> 7.1.428.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.1.428.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.1.428 <br> 7.1.428 <br> 4.1.428 <br> 4.1.428 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15 <br> 10.0.0 |

## Contents 

Microsoft Azure Service Fabric 7.1 First Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Repositories and Download Links](#repositories-and-download-links)

## Key Announcements
**Potential 7.1 Deployment Failures**:<br>
**Cause**: SF 7.1 introduced a more rigorous validation of security settings; in particular, requiring that settings ClusterCredentialType and ServerAuthCredentialType have matching values. However, existing clusters may have been created with 'x509' for the ServerAuthCredentialType and 'none' for the ClusterCredentialType. <br> **Impact**:  In the case mentioned above, attempting an upgrade to SF71CU1 will cause the upgrade to fail. <br> **Workaround** :No workaround exists for this issue, as the ClusterCredentialType is immutable. If you are in this situation, please continue using SF70 until the 7.1 second refresh release becomes available.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows  7.1.417.9590** | **Bug** |GetDeployedCodePackageListAsync API breaking change fix|**Brief desc**:  There was an API change in SF 7.1 and SF 7.1CU1 to the function "GetDeployedCodePackageListAsync()" by adding an extra boolean variable. However, the following API was taken out by accident making it a breaking change for anyone who used it previously. <br> **Impact**:  Anyone using the API "Get-DeployedCodePackageListAsync" and upgraded to 7.1 or 7.1CU2 would run into a "Method Not found" error if their app used the above API.[**Documentation**](https://docs.microsoft.com/dotnet/api/system.fabric.fabricclient.queryclient.getdeployedcodepackagelistasync?view=azure-dotnet) <br> **Workaround**: Recompile your app that uses the above API to use the new signatures which include a boolean for includeCodePackageUsageStatistics and redeploy those apps in your cluster. <br> **Fix**: This fix re-adds the original API signatures.
| **Windows  7.1.417.9590** | **Bug** |Investigate Certificate ACLing for potential memory leaks|**Brief desc**: SF 7.1 introduced periodic monitoring and ACLing of application certificates declared by common name; this exposed (and exacerbated) an existing memory leak in the certificate ACLing code, causing a more rapid increase in memory pressure. <br> **Impact**:  FabricHost.exe leaks OS-allocated security descriptors at a rate of roughly 1kb/minute/application certificate. Frequent application updates or sharing of the certificates results in a higher leak rate. <br> **Workaround**: Options include: <br> restarting the SF runtime on the node at regular intervals <br> avoiding declaring endpoint certificates by CN <br> staying on SF 7.0 builds <br> **Fix**: This fix re-adds the original API signatures.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.1.428.1 | N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.1.428.9590 | N/A | https://download.microsoft.com/download/0/3/4/0344ed86-f29f-47a0-9de2-67424644e773/MicrosoftServiceFabric.7.1.428.9590.exe |
| Service Fabric for Windows Server | Service Fabric Standalone Installer Package | 7.1.428.9590 | N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.1.428.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.1.428.9590.zip |
|| Service Fabric Standalone Runtime | 7.1.428.9590 | N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.1.428.9590/MicrosoftAzureServiceFabric.7.1.428.9590.cab |
|.NET SDK | Windows .NET SDK | 4.1.428 | N/A | https://download.microsoft.com/download/0/3/4/0344ed86-f29f-47a0-9de2-67424644e773/MicrosoftServiceFabricSDK.4.1.428.msi |
||Microsoft.ServiceFabric | 7.1.428 | N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.1.428 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.1.428 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.1.428 | N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.1.428 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A | https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A | N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 | https://github.com/Azure/generator-azuresfjava | N/A |
||Azure Service Fabric C# generator | 1.0.9 | https://github.com/Azure/generator-azuresfcsharp | N/A |
||Azure Service Fabric guest executables generator | 1.0.1 | https://github.com/Azure/generator-azuresfguest | N/A |
||Azure Service Fabric Container generators | 1.0.1 | https://github.com/Azure/generator-azuresfcontainer | N/A |
| CLI |Service Fabric CLI | 10.0.0 | https://github.com/Azure/service-fabric-cli | https://pypi.python.org/pypi/sfctl |
| PowerShell | AzureRM.ServiceFabric | 0.3.15 | https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric | https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |


 
