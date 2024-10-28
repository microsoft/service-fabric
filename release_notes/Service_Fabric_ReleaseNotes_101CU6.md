# Microsoft Azure Service Fabric 10.1 Cumulative Update 6.0 Release Notes

This release will only be available through Auto upgrades. Clusters set to automatic upgrades will receive this release. For how to configure upgrades, please see [classic](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-upgrade) or [managed](https://docs.microsoft.com/en-us/azure/service-fabric/how-to-managed-cluster-configuration) documentation.

## Contents
* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions

The following packages and versions are part of this release:

| **Service** | **Platform** | **Version** |
|---|---|---|
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.10.1.2493.9590.exe) | Ubuntu 20 <br> Windows | 10.1.2319.1 <br> 10.1.2493.9590 |
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/10.1.2493.9590/Microsoft.Azure.ServiceFabric.WindowsServer.10.1.2493.9590.zip) | Service Fabric Standalone Installer Package | 10.1.2493.9590 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.1.2493.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 7.1.2175 <br> 10.1.2175 <br> 7.1.2175 <br> 7.1.2175 |
| [Java SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.1.2493.msi) | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module<br>SFCTL | 0.3.15 <br> 11.0.1 |

## Service Fabric Feature and Bug Fixes

| **Versions** | **Issue Type** | **Description** | **Resolution** |
|---|---|---|---|
| Windows - <br> 10.1.2493.9590 <br> Ubuntu 20 - <br> 10.1.2319.1 | Bug | Service Fabric Runtime for Linux | **Brief Description:** Service Fabric Runtime for Linux updated its .NET dependency from 6 to 8.  This caused .NET 8 to be installed on the system instead of .NET 6. <br> **Feature/Bug Impact:** This makes it so Service Fabric for Linux continues to use a support .NET. <br> **Solution/Fix:** Updates the dependencies that Service Fabric for Linux has on installing its DEB package to specify .NET 8 packages instead of .NET 6 packages. <br> **Breaking Change:** It's possible this will break customers who aren't installing .NET 6 to support their own applications. |
| Windows - <br> 10.1.2493.9590 <br> Ubuntu 20 - <br> 10.1.2319.1 | Bug | BRS | **Brief Description:** User Services with BRS enabled and running as elevated privileges as SYSTEM, will have BRS use the OS drive to temporarily write Compressed backups. This happened after [a .NET security upgrade](https://support.microsoft.com/topic/july-9-2024-kb5041016-cumulative-update-for-net-framework-3-5-4-8-and-4-8-1-for-windows-server-2022-6454ceab-7a1e-4d9e-8f56-385bee0054d7) that changed behavior of GetTempPath API that BRS relies on (backups get written to temp path before getting copied to Az storage). <br> **Feature/Bug Impact:** BRS backups were getting written to OS disk (smaller) instead of data disk (larger) where it usually goes. Disk filled up faster than what BRS could clean up (post uploading to Az storage). <br> **Solution/Fix:** BRS to use CodeActivationContext.TempFolder provided by Service Fabric Hosting instead of GetTempPath. <br> **Workaround:** Run user services as non-system. <br> **Temporary Workaround:** Opt out of the .NET security update. *Warning:* The opt-out will disable the security fix for the elevation of privilege vulnerability detailed in [CVE 2024-38081](https://msrc.microsoft.com/update-guide/vulnerability/CVE-2024-38081). The opt-out is only for temporary workaround if you're sure that the software is running in secure environments. We don't recommend applying this temporary workaround. If you still wish to opt out, you have two options: opt out in a command window by setting the environment variable `COMPlus_Disable_GetTempPath2=true` or [opt out globally on the machine by creating a system-wide environment variable and rebooting to ensure all processes observe the change](https://stackoverflow.com/questions/2365307/what-determines-the-return-value-of-path-gettemppath). System-wide environment variables can be set by running `sysdm.cpl` from a command window and navigating to `Advanced -> Environment variables -> System variables -> New`. |

## Retirement and Deprecation Path Callouts

* Service Fabric (SF) runtime will discontinue support for the Java SDK soon. For a smooth transition, we strongly recommend users to shift to Azure Service Fabric .NET SDK. If your current setup is based on the Service Fabric Java SDK, we suggest starting migration plans to smoothly switch to the Azure Service Fabric .NET SDK. Although applications using the Java SDK will continue to work, we highly recommend adopting the SF .NET SDK for optimal outcomes.

* Ubuntu 18.04 LTS reached its 5-year end-of-life window on June-2023. Service Fabric runtime has dropped support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](/azure/service-fabric/service-fabric-versions)

* Previously communicated, Service Fabric runtime had planned to remove Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older, from the package Download Center in July 2023. We would like to inform you that this timeline has been extended, and the removal will now take place in January 2024.

* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.
  * To prevent disruption of workloads, create a new cluster using the following steps:
    * [Create a Service Fabric cluster using ARM template](/azure/service-fabric/quickstart-cluster-template)
    * [Create a Standalone cluster](/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)
    * Install the supported version of Service Fabric SDK based on the Runtime version installed on the cluster.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

Run Time:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.10.1.2493.9590.exe

SDK:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.1.2493.msi

Cab:
https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/10.1.2493.9590/MicrosoftAzureServiceFabric.10.1.2493.9590.cab
 
Package:
https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/10.1.2493.9590/Microsoft.Azure.ServiceFabric.WindowsServer.10.1.2493.9590.zip
 
Goalstate:
https://download.microsoft.com/download/7/D/1/7D1D1511-59A4-4933-8187-40C20065AA29/10.1.2493.9590/goalstate.10.1.2493.9590.json
