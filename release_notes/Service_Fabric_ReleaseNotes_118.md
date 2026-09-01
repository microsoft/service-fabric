# Microsoft Azure Service Fabric 11.8 Release Notes

This release will only be available through Auto upgrades. Clusters set to automatic upgrades will receive this release. For how to configure upgrades, please see [classic](https://docs.microsoft.com/azure/service-fabric/service-fabric-cluster-upgrade) or [managed](https://docs.microsoft.com/azure/service-fabric/how-to-managed-cluster-configuration) documentation.

## Contents
* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Service Fabric Features and Bug Fixes](#service-fabric-features-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions

Packages and versions are listed with the most recent version listed first.

The following packages and versions are part of this release:

### Service Fabric 11.8.121

| **Service** | **Platform** | **Version** |
|---|---|---|
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.8.121.1.exe) | Windows <br> Windows ARM64 <br> Ubuntu 22 | 11.8.121.1 <br> 11.8.121.2 <br> 11.8.121.4 |
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.8.121.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.8.121.1.zip) | Service Fabric Standalone Installer Package | 11.8.121.1 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.8.121.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 8.0.0 <br> 11.8.121.1 <br> 8.0.0 <br> 8.0.0 |

## Service Fabric Features and Bug Fixes

Features and bug fixes are listed by the version in which they were introduced, with the most recent version listed first.

The following features and bug fixes are part of this release:

### Service Fabric 11.8.121 Features and Bug Fixes

| **Type** | **Description** | **Impact and Resolution** |
|---|---|---|
| Feature | Partitions could be permanently lost during FM rebuild if all replicas were deleted before a replacement was placed. This fix delays the deletion of the last replica to ensure partition metadata is preserved. | **Impact:** If all replicas of a stateful partition were dropped before Failover Manager rebuilt and placed replacements, no node could report the partition during the Reconfiguration Agent Local Failover Unit Map upload. The partition metadata could then be permanently lost. <br> **Solution/Fix:** Failover Manager no longer deletes the last replica of a partition until a replacement replica is running and ready. This behavior is gated by the DelayDeleteOfLastReplica setting, which defaults to false. |
| Feature | Renamed the folder within SAWDrop that the Internal ServiceFabric PowerShell module is copied to during the build procedure. Without this change, the Internal ServiceFabric PowerShell module is copied into a folder whose name collides with the folder holding the Public ServiceFabric PowerShell module. The result is that Service Fabric DRIs who want to use the Internal module on their SAW need to perform an additional manual step to import the module present in the SAWDrop every time they want to use an internal cmdlet. With this change, the additional manual step is no longer needed. | **Impact:** The internal and public Service Fabric PowerShell modules used folders with conflicting names in SAWDrop. Service Fabric engineers had to manually import the internal module each time they used an internal cmdlet. <br> **Solution/Fix:** Renamed the internal module's build folder in SAWDrop to avoid the naming conflict and manual import step. |
| Feature | Updated multiple dependencies of Service Fabric, such as to address CVEs in those dependencies. We don't believe that SF is impacted by the CVEs listed, but we have updated the dependencies as a best practice. | **Impact:** Security scanners might report vulnerabilities in third-party libraries installed on Service Fabric nodes, although Service Fabric isn't believed to be affected by the listed CVEs. <br> **Solution/Fix:** Updated OpenSSL through gRPC 2.6.2 and OpenSSL 3.2.6 to address CVE-2024-4741, CVE-2024-6119, and CVE-2025-9230. Updated libcurl through Azure.Blob.Cpp.Client.SF 12.13.2 and libcurl 8.20.0 to address CVE-2026-7168, CVE-2026-6429, CVE-2026-6276, and CVE-2025-14017. These dependency updates don't include breaking changes. |

## Retirement and Deprecation Path Callouts

* Service Fabric runtime will discontinue support for the Java SDK soon. For a smooth transition, we strongly recommend users to shift to Azure Service Fabric .NET SDK. If your current setup is based on the Service Fabric Java SDK, we suggest starting migration plans to smoothly switch to the Azure Service Fabric .NET SDK. Although applications using the Java SDK will continue to work, we highly recommend adopting the Service Fabric .NET SDK for optimal outcomes.

* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving and removing will affect application scaling and re-imaging of virtual machines in a Service Fabric cluster running on unsupported versions. After older versions are removed/archived, this may cause rollback failures when an in-progress upgrade has errors.
  * To prevent disruption of workloads, create a new cluster using the following steps:
    * [Create a Service Fabric cluster using ARM template](https://learn.microsoft.com/azure/service-fabric/quickstart-cluster-template)
    * [Create a Standalone cluster](https://learn.microsoft.com/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)
    * Install the supported version of Service Fabric SDK based on the runtime version installed on the cluster.

## Repositories and Download Links

The list below is an overview of the direct links to the packages associated with this release, with the links for the most recent version listed first.

Follow this guidance for setting up your developer environment:
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

Runtime:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.8.121.1.exe

SDK:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.8.121.msi

Cab:
https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/11.8.121.1/MicrosoftServiceFabric.11.8.121.1.cab

Package:
https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.8.121.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.8.121.1.zip

Goalstate:
https://download.microsoft.com/download/7/d/1/7d1d1511-59a4-4933-8187-40c20065aa29/11.8.121.1/goalstate.11.8.121.1.json