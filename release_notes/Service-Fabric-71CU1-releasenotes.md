# Microsoft Azure Service Fabric 7.1 First Refresh Release Notes

**The rollout for this release has been resumed. Please expect to recieve the update in the next 7-10 days.**

This release includes bug fixes and features as described in this document. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | 7.1.418.1 <br> 7.1.417.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.1.417.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.1.416 <br> 7.1.417 <br> 4.1.417 <br> 4.1.417 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15 <br> 10.0.0 |

## Contents 

Microsoft Azure Service Fabric 7.1 First Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Repositories and Download Links](#repositories-and-download-links)

## Key Announcements

* **Cluster Upgrades**: Automatic upgrades will resume as of this release. Any clusters set to automatic upgrades will pick up 7.1CU1 as soon it becomes available in the region. Clusters that have not already upgraded from 7.0 to 7.1 will upgrade directly to 7.1CU1.  
* [**General Availability of Request Drain**](https://docs.microsoft.com/azure/service-fabric/service-fabric-application-upgrade-advanced#avoid-connection-drops-during-planned-downtime-of-stateless-services): During planned service maintenance, such as service upgrades or node deactivation, you would like to allow the services to gracefully drain connections. This feature adds an instance close delay duration in the service configuration. During planned operations, SF will remove the Service's address from discovery and then wait this duration before shutting down the service.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows  7.1.417.9590** | **Bug** |Central Secret Service does not handle transaction failures|**Brief desc**:  Prior to SF71CU1, the Central Secure Store service could fail on concurrent transactions, potentially leaving stored entities in an inconsistent state.  <br> **Impact**: Secrets with concomitant PUTs and DELETEs may silently fail, leaving the secret in an inconsistent state. <br> **Workaround**: Avoid parallel opposing (PUT/DELETE) operations on the same entity. <br> **Fix**:  CSS enforces uniqueness checks and rolls back failed transactions.
| **Ubuntu 7.1.418.1** | **Bug** |Security setting validator fails incorrectly on Linux|**Brief desc**:  The SF functionality which attempts to pre-validate security settings, during an initial cluster deployment or upon changing its security settings, incorrectly marked cluster certificates as non-existent on Linux.   <br> **Impact**: Setting the Security\EnforcePrevalidationOnSecurityChanges setting to true on Linux will cause cluster upgrades/deployments to fail. <br> **Workaround**: Do not enable this setting (by default it is disabled). <br> **Fix**:  Disable the setting (its default value is false)
| **Windows  7.1.417.9590** | **Bug** |Exclude app endpoint certs declared by TP from monitoring|**Brief desc**:   SF71 introduced support for declaring app endpoint certs by subject common name; to enable auto-rollover, these certificates must be monitored for renewal and ACL’d accordingly. Prior SF71, endpoint certificates had been declared by thumbprint only, and were not ACLd explicitly by SF code. Instead, the SF runtime would look up the certificate by thumbprint, ACL the endpoint port and then bind the cert to the port. After SF71, all endpoint certificates are pre-ACLd as part of deploying an application to a node, which can cause a failure of application activation or upgrade in the following scenarios: <br>**(a)** Applications which used certs not supported by SF but supported by http.sys: Elliptic Curve certs, or certs with private keys managed by CAPI2 (CNG certs) </br>**(b)** Applications with multiple services and multiple endpoint certificates, which used placement restrictions to separate the services among node types, and did not provision all certificates to all nodes.<br> **Impact**:in either of the scenarios above, application activation will fail. This may happen in the normal course of operation, or during a cluster or application upgrade. In that case, the upgrades will roll back. <br> **Workaround**:  for scenario (a) above, there is no feasible workaround. If possible, though, consider using RSA certificates with private keys managed by a CSP (CryptoServiceProvider, CAPI1). If this is not possible, please do not upgrade to SF71, go directly to SF71CU1. For scenario (b) above, install all declared endpoint certificates on all nodes where the application is deployed, irrespective of the segregation of the services comprising the application. <br> **Fix**: SF71CU1 exempts endpoint certificates declared by thumbprint from explicit/required ACLing as part of app activation - in essence it restores the pre-SF71 behavior.
| **Windows  7.1.417.9590** | **Bug** |Upload files to ImageStore creates directory marker files in current directory|**Brief desc**:  Registering application (a.k.a. Provision) creates temporary files in current directory. The access to current directory may be impacted by runtime environment. The client's process might change the current directory during uploading or the client may not have write access to the running directory. <br> **Impact**: Uploading or registering application package could fail by errors like ACCESSDENIED or FileNotFound <br> **Workaround**: A client that uses FabricClient ensures that the write access is granted to the process through uploading and registering application package. <br> **Fix**: The working directory is changed from current directory to process's temporary directory.
| **Windows  7.1.417.9590** | **Bug** |ImageStoreService primary recovers only one file among all missing files|**Brief desc**:ImageStoreService doesn't recover every file when new primary replica detects missing files in storage. The ImageStoreService becomes healthy with missing files. <br> **Impact**: Application will fail to activate replica on nodes that can't download required files. <br> **Workaround**: A client that uses FabricClient ensures that the write access is granted to the process through uploading and registering application package. <br> **Fix**: ImageStoreService primary copies every file from secondary replica if any missing file in file system is detected. 
| **Windows  7.1.417.9590** | **Bug** |InstallFabricRuntime: UAC doesn't appear to grant all necessary privileges for install|**Brief desc**:Users launching the SF runtime installer were hitting a jarring experience where the launcher would start and immediately close. This is because it is intended to be launched with an /accepteula parameter and since the executable would launch in a separate User Account Control (UAC) console it would close before the user had a chance to understand the intended usage pattern in the exit message. <br> **Impact**: Users installing SF runtime for as part of SDK dev machine setup, outside of the WebPI experience. <br> **Workaround**: Run the runtime installer exe from administrator cmd console. This will self-describe the required parameters. <br> **Fix**: We are now detecting when the executable is launched in a way where the program output would not get a chance to be read, and waiting for first of 1) user input or 2) 30 seconds before closing.
| **Windows  7.1.417.9590** | **Bug** |Race condition in node configuration read results in upgrade operation hang since config change handler never fires|**Brief desc**:Fabric.Package.Current.xml value for NodeVersion changes when FabricDeployer InstanceId or Update operation runs, and Fabric upgrade subscribes to the configuration changed event. There is a rare case in the PAASv1 environment where the config value shifts from value A->B->A quickly due to unsynchronized back-to-back FabricDeployer operations between the Fabric runtime and InfrastructureWrapper. Due to an aliasing effect caused by a time gap in between receiving a file changed notification and the read of the file the value is interpreted as unchanged, so the upgrade handler never fires. <br> **Impact**: When an instance id upgrade rolls to a PAASv1 node which is going through an infrastructure specification update at the same time, the node may be wedged in an intermediate state. If MR is not enabled for the cluster, this may lead to quorum loss.<br> **Workaround**:Restart Fabric on the node. <br> **Fix**: Add a timeout for the upgrade NodeVersion config changed notification, in order to unblock the upgrade cycle.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 7.1.418.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.1.417.9590 | N/A | https://download.microsoft.com/download/a/4/e/a4e59def-7e11-4a00-b43a-4f769d89129b/MicrosoftServiceFabric.7.1.417.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 7.1.417.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.1.417.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.1.417.9590.zip |
||Service Fabric Standalone Runtime | 7.1.417.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.1.417.9590/MicrosoftAzureServiceFabric.7.1.417.9590.cab |
|.NET SDK |Windows .NET SDK | 4.1.416 |N/A | https://download.microsoft.com/download/a/4/e/a4e59def-7e11-4a00-b43a-4f769d89129b/MicrosoftServiceFabricSDK.4.1.416.msi |
||Microsoft.ServiceFabric | 7.1.417 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 4.1.417 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 4.1.417 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.1.417 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.1.417 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A | https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 10.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |


 
