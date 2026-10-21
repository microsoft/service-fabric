# Microsoft Azure Service Fabric 11.8 Release Notes

This release will be available through automatic upgrades. Clusters configured for automatic upgrades will receive it according to their upgrade policy. For upgrade configuration guidance, see the [classic cluster](https://learn.microsoft.com/azure/service-fabric/service-fabric-cluster-upgrade) or [managed cluster](https://learn.microsoft.com/azure/service-fabric/how-to-managed-cluster-configuration) documentation.

## Contents
* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Key Announcements](#key-announcements)
* [Breaking Changes](#breaking-changes)
* [Service Fabric Features and Bug Fixes](#service-fabric-features-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions

The following packages and versions are part of this release:

### Service Fabric 11.8.121

| **Service** | **Platform** | **Version** |
|---|---|---|
| Service Fabric Runtime | Windows <br> ARM64 <br> Ubuntu 22 <br> AzLinux | 11.8.121.1 <br> 11.8.121.2 <br> 11.8.121.4 <br> 11.8.131.5|
| Service Fabric for Windows Server | Service Fabric Standalone Installer Package | 11.8.121.1 |
| .NET SDK | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 8.0.0 <br> 11.8.121.1 <br> 8.0.0 <br> 8.0.0 |
| Java SDK | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module <br> SFCTL | 0.3.15 <br> 11.0.1 |

## Key Announcements

Service Fabric 11.8 improves reliability across Reliable Collections, service resolution, and Failover Manager. It also adds configurable standby-replica promotion, Native SDK support for partition endpoint version serialization, and updated native dependencies containing the latest security fixes.

## Breaking Changes

No customer-impacting breaking changes are identified in this release. Review the behavior changes and configuration guidance in the feature and bug fix table before upgrading.

## Service Fabric Features and Bug Fixes

The following features and bug fixes are part of this release:

### Service Fabric 11.8.121 Features and Bug Fixes

| **Type** | **Description** | **Impact and Resolution** |
|---|---|---|
| Feature | Configurable standby-replica promotion during replica shortages | **Impact:** Partitions can recover replica count sooner instead of waiting for down replicas or restart timeout. <br> **Solution/Fix:** Service Fabric adds the dynamic `FailoverManager/EnableStandByPromotionOnReplicaShortage` setting, which is enabled by default. It allows available standby replicas to be promoted for faster recovery when a partition is below its target replica count. Customers who prefer greater placement stability and fewer replica movements can disable the setting while keeping the service operational. |
| Bug fix | Improved TStore reliability during background maintenance | **Impact:** Improves reliability for Reliable Collections under concurrent reads and store maintenance. <br> **Solution/Fix:** TStore now synchronizes value access with background sweeping during reads, checkpointing, and serialization. This prevents object-lifetime races and improves Reliable Collections stability under concurrent workloads. No action is required. |
| Bug fix | More consistent service resolution after Failover Manager failover | **Impact:** Prevents lookup-version reuse after Failover Manager failover, improving service-resolution consistency. <br> **Solution/Fix:** Service Fabric now preserves the highest lookup version when partition metadata is deleted. This prevents lookup-version reuse after a Failover Manager failover and improves service-resolution consistency. No action is required. |
| Bug fix | Protection against partition metadata loss during Failover Manager rebuild | **Impact:** Reduces the risk of stateful partitions being lost when Failover Manager rebuilds happen. <br> **Solution/Fix:** Service Fabric can delay deletion of the last local replica until a replacement is available, reducing the risk of losing stateful partition metadata during a Failover Manager rebuild. This fix is controlled by the `DelayDeleteOfLastReplica` configuration setting. It is disabled by default in this release and is planned to be enabled by default in the following release. |
| Security update | Native dependency security updates | **Impact:** Customers benefit from native dependencies that include their latest security fixes; no action is required. <br> **Solution/Fix:** Service Fabric now includes updated native dependencies that incorporate their latest security fixes. No customer action is required. |

## Retirement and Deprecation Path Callouts

* Service Fabric plans to discontinue support for the Java SDK. Applications that use the Java SDK will continue to run, but customers should plan migration to the Service Fabric .NET SDK.

* Service Fabric also plans to archive runtime versions earlier than 7.2 and SDK versions 4.2 and earlier. To avoid scaling, reimaging, or rollback disruption, move workloads to a supported runtime and SDK version.

## Repositories and Download Links

* [Service Fabric documentation](https://learn.microsoft.com/azure/service-fabric/)
* [Service Fabric samples and source repositories](https://github.com/microsoft/service-fabric)
* [Service Fabric release information](https://learn.microsoft.com/azure/service-fabric/release-notes)

### Service Fabric 11.8.121 Repositories and Download Links

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
