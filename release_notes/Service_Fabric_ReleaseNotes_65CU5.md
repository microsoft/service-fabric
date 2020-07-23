# Microsoft Azure Service Fabric 6.5 Fifth Refresh Release Notes

This release includes the bug fixes and features described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | 6.5.476.1 <br> 6.5.676.9590   |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 6.5.676.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration|3.4.676  <br> 6.5.676 <br> 3.4.676 <br> 3.4.676 |
|Java SDK  |Java for Linux SDK  | 1.0.5 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 8.0.0 |
|Service Fabric Tooling |Visual Studio Tooling | 2.5.20615.1 |

## Contents 

Microsoft Azure Service Fabric 6.5 Fifth Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Service Fabric Common Bug Fixes](#service-fabric-common-bug-fixes)
* [Repositories and Download Links](#repositories-and-download-links)
* [Visual Studio 2015 Tool for Service Fabric - Localized Download Links](#visual-studio-2015-tool-for-service-fabric-\--localized-download-links)


## Service Fabric Runtime

| Versions | IssueType | Description | Resolution | 
|----------|-----------|-|-|
| **Windows 6.5.676.9590       <br> Ubuntu 6.5.476.1** | **Bug** | Memory leak when calling the FabricClient.QueryManager.GetDeployedReplicaListAsync API | **Impact**:  Memory leak occurs whenever the FabricClient.QueryManager.GetDeployedReplicaListAsync API is called. <br>**Fix**:  Identified the leaking code and fixed it to free the memory when no longer in use. <br> **Workaround**: None |
| **Windows 6.5.676.9590 <br> Ubuntu 6.5.476.1** | **Bug** | The list of deactivation tasks returned in node query result has some extra invalid entries | **Impact**: Usability issue. The user sees some invalid entries in the node query result.  e.g. the result returned by the Get-ServiceFabricNode PowerShell cmdlet. <br>**Fix**:  Fixed the code to not insert those invalid entries into the node query result. <br> **Workaround**: Ignore the invalid entries. The deactivation task type for those invalid entries would be "Invalid". Such entries could be ignored. |
| **Windows 6.5.676.9590 <br> Ubuntu 6.5.476.1** | **Bug** | The default timeout (10 minutes) for WaitForReconfiguration and WaitForInBuildReplica safety checks during upgrades and node deactivations is too short. | **Impact**: In some scenarios, the failure of WaitForReconfiguration and WaitForInBuildReplica safety checks implies that the upgrade or node deactivation would cause availability loss if it were allowed to proceed. The timeout for these two safety checks is 10 minutes by default. After the timeout period elapses, Service Fabric allows the upgrade or node deactivation to proceed even if the safety checks are failing. In some scenarios, this can lead to availability loss for the partition that is failing the safety check.  <br>**Fix**: The default timeout for the WaitForReconfiguration and WaitForInBuildReplica safety checks has been made infinite to avoid availability loss situations. <br> **Workaround**: Change the default timeout to a very high value using the WaitForReconfigurationSafetyCheckTimeout and WaitForInBuildReplicaSafetyCheckTimeout parameters in the FailoverManager section of cluster settings. https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings#failovermanager |
| **Windows 6.5.676.9590   <br> Ubuntu 6.5.476.1** | **Bug** | Provisioning an application strips native DLL files off user application package, when they are common with Service Fabric runtime. | **Impact**:  Application can have an error in runtime by wrong version of binaries. <br>**Fix**:  gRPC binaries are added to whitelist that doesn't filter out from user application package. <br> **Workaround**: None |
| **Windows 6.5.676.9590** | **Feature** | Enable the user to keep the DNS suffix empty |**Brief desc**: Previously Service Fabric would append the service name as a suffix to a DNS name by default. Previously there was no way to override this setting. Now the user can use a hosting setting so that no suffix will be appended. <br> **How/When to Consume it**: In fabric settings, set the Hosting parameter DefaultDnsSearchSuffixEmpty to true.|
| **Windows 6.5.676.9590** | **Bug** |Upgrade failure 6.4->6.5 rollback caused DNS to no longer resolve remote queries|**Brief desc**: There is a corner case where if 6.5 doesn't clean-up DNS correctly 6.4 will open DNS and cause a corruption of the DNS server list such that it only has the SF DNS IP address (localhost IP) and can no longer resolve remote queries. <br> **Impact**: This will cause DNS resolution to completely fail for remote query. <br> **Workaround**: Manually correct the DNS server list to include a remote DNS server such as Azure DNS. | 

## Service Fabric Common Bug Fixes


| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 6.5.676.9590** | **Bug** |Improve DNS intermittent resolve errors|**Brief desc**: Intermittent resolve shows up as short periods of time where a resolve will timeout or be unable to correctly statisfy a request.  <br> **Impact**: Causes interruptions in service for customers. <br> **Workaround**: Wait for it to self correct or apply a patch script. <br> **How/When to Consume it**: In fabric settings, set the Hosting parameter DnsServerListTwoIps to true. **Documentation: https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings#dnsservice| 

## Service Fabric Explorer
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 6.5.676.9590   <br> Ubuntu 6.5.476.1** | **Feature** | Event store viewer timeline|**Brief desc**: A scrollable timeline has been added to the event store viewer for cluster upgrades, application upgrades and viewing nodes in down state. https://github.com/microsoft/service-fabric-explorer/issues/219| 
| **Windows 6.5.676.9590   <br> Ubuntu 6.5.476.1** | **Bug** |Increased  usability  and  conveniences for Service fabric explorer  |**Brief desc**: Expand all/close all on some parts of the tree navigation. Banner level warnings for cluster cert expirations. Links between replicas on applications and node pages. <br> https://github.com/microsoft/service-fabric-explorer/pull/217 <br> https://github.com/microsoft/service-fabric-explorer/pull/212 <br> https://github.com/microsoft/service-fabric-explorer/issues/213 | 
| **Windows 6.5.676.9590   <br> Ubuntu 6.5.476.1** | **Bug** | Tables overlapping and upgrade banner disappearing  |**Brief desc**: Some tables in newer versions of chrome and Chromium based Edge would overlap and be hard to view. The upgrade banner would disappear for parts of the upgrade process. <br> https://github.com/microsoft/service-fabric-explorer/pull/223 <br> https://github.com/microsoft/service-fabric-explorer/issues/214 | 

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up |6.5.476.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 6.5.676.9590 | N/A |https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.6.5.676.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package |6.5.676.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/6.5.676.9590/Microsoft.Azure.ServiceFabric.WindowsServer.6.5.676.9590.zip |
||Service Fabric Standalone Runtime |6.5.676.9590 |N/A |https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/6.5.676.9590/MicrosoftAzureServiceFabric.6.5.676.9590.cab   |
|.NET SDK |Windows .NET SDK |3.4.676 |N/A |https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.3.4.676.msi  |
||Microsoft.ServiceFabric |6.5.676 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf |3.4.676|https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*|3.4.676 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal |3.4.676 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions |3.4.676 |N/A |https://www.nuget.org |
|Java SDK |Java SDK |1.0.5 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.5 |
|Visual Studio |Visual Studio 2017 Tools for Service Fabric 
2.5.20608.1 |N/A |Included in Visual Studio 2017 Update 7 (15.7) and above |
|Visual Studio 2015 Tools for Service Fabric |2.5.20615.1 |N/A |See localized download links below |
|Eclipse |Service Fabric plug-in for Eclipse |2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator |1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator |1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator |1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators |1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI |8.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric |0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |

## Visual Studio 2015 Tool for Service Fabric - Localized Download Links​

**The below download links are for the 2.5.20615.1 release of Visual Studio 2015 Tools for Service Fabric.**
 
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.de-de.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.en-us.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.es-es.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.fr-fr.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.it-it.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.ja-jp.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.ko-kr.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.ru-ru.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.zh-cn.msi
* https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftAzureServiceFabricTools.VS140.zh-tw.msi
