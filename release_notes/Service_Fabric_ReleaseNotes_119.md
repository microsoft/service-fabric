# Service Fabric 11.9 Release Notes

This release will be available through automatic upgrades. Clusters configured for automatic upgrades will receive it according to their upgrade policy. For upgrade configuration guidance, see the [classic cluster](https://learn.microsoft.com/azure/service-fabric/service-fabric-cluster-upgrade) or [managed cluster](https://learn.microsoft.com/azure/service-fabric/how-to-managed-cluster-configuration) documentation.

## Contents
* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Key Announcements](#key-announcements)
* [Breaking Changes](#breaking-changes)
* [Service Fabric Features and Bug Fixes](#service-fabric-features-and-bug-fixes)
* [Retirement and Upgrade Callouts](#retirement-and-upgrade-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions

The following packages and versions are part of this release:

### Service Fabric 11.9

| **Service** | **Platform** | **Version** |
|---|---|---|
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.9.121.1.exe) | Windows <br> ARM64 <br> Ubuntu 22 <br> AzLinux | 11.9.121.1 <br> 11.9.121.2 <br> 11.9.121.4 <br> 11.9.122.5|
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.9.121.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.9.121.1.zip) | Service Fabric Standalone Installer Package | 11.9.121.1 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.9.121.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 8.9.121 <br> 11.9.121.1 <br> 8.9.121 <br> 8.9.121 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module <br> SFCTL | 0.3.15 <br> 11.0.1 |

## Key Announcements

Service Fabric 11.9 improves Reliable Collections recovery, Linux runtime stability, transport shutdown, partition queries, and service placement. Backup and Restore Service supports `Premium_LRS` `BlockBlobStorage` accounts without an Azure Table endpoint. Review the read-ahead cache validation limit and locking-metric dimension changes before upgrading.

## Breaking Changes

Upgrade considerations: `ReadAheadCacheSizeInKb` values above 2,097,148 KiB are now rejected; reduce higher values before upgrading or submitting configuration. The default remains 1,024 KiB. `ResourceNameHash` is removed from `LockAcquired`, `LockAcquisitionFailure`, and `LockHeldDuration` metrics; update queries and dashboards that use that dimension. Review the table for other behavior changes and diagnostic event-consumer guidance.

Before upgrading Linux clusters to Service Fabric 11.9, validate and correct all explicitly configured TLS 1.3 cipher suites, or remove the explicit cipher suite configuration to use the system defaults.

## Service Fabric Features and Bug Fixes

The following public features, behavior changes, reliability updates and bug fixes are part of this release:

| **Type** | **Description** | **Impact and Resolution** |
|---|---|---|
| Bug fix | Stateful Reliable Services compatibility on ARM64 | **Impact:** Stateful Reliable Services hosted by an emulated x64 application on an ARM64 node can open without the managed assembly architecture mismatch. <br> **Solution/Fix:** Fixed a managed assembly architecture mismatch that could prevent stateful Reliable Services from opening when an x64 application host runs under emulation on an ARM64 node. No application API changes are required. |
| Bug fix | Simplified Reliable Collections locking metrics | **Impact:** Per-key `ResourceNameHash` metric series are no longer emitted. Queries using this dimension need updating; locking behavior is unchanged. <br> **Solution/Fix:** Reduced the cardinality of managed locking metrics by removing the `ResourceNameHash` dimension from `LockAcquired`, `LockAcquisitionFailure`, and LockHeldDuration. The metrics remain available by `LockMode`, and acquisition failures also retain Reason. Lock semantics are unchanged. Update metric queries or dashboards that group or filter these metrics by ResourceNameHash. |
| Reliability update | More reliable Reliable Collections shutdown and KTL service diagnostics | **Impact:** Background lock cleanup is better coordinated with shutdown. Existing applications need no changes. <br> **Solution/Fix:** Reliable Collections improves synchronization between background lock cleanup and service shutdown. KTL asynchronous service implementations gain protected accessors, `IsDeferredClosePending` and `GetPendingServiceActivityCount`, for deferred-close state and pending service activities. No application changes are required for existing usage. |
| Behavior change | Upper limit for the transactional replicator read-ahead cache | **Impact:** `ReadAheadCacheSizeInKb` values above 2,097,148 KiB are rejected. The default remains 1,024 KiB. <br> **Solution/Fix:** Added an upper limit of 2,097,148 KiB for `ReadAheadCacheSizeInKb` in native TransactionalReplicator2 configuration validation to keep aligned read-ahead allocations below 2 GiB. Configurations specifying higher values are now rejected during validation. The default remains 1,024 KiB. Before upgrading or submitting a cluster configuration, reduce any value above the maximum to a supported value. |
| Bug fix | Improved Linux runtime and transaction reliability | **Impact:** Improves asynchronous operation and transaction-commit reliability, including timeout and cancellation handling. <br> **Solution/Fix:** Improved Linux runtime reliability during asynchronous callbacks, transport and lease cleanup, and native transaction commits. Transaction commit timeout and cancellation state now remain valid while asynchronous work is pending. No application or configuration changes are required. |
| Reliability update | Reliable Collections memory accounting and recovery diagnostics | **Impact:** Improves consistency during concurrent reads and maintenance, and exposes a health warning if the sweep manager cannot start. <br> **Solution/Fix:** Reliable Collections improves memory-accounting consistency during concurrent reads, sweeping, and consolidation, and adds native state-provider sweep support interfaces. Native transactional replicas can now report a `SweepManagerUnavailable` health warning when background sweep-manager initialization fails. Existing store factories retain their `ReadOrWrite` sweep policy. No customer configuration changes are required. |
| Bug fix | Safer handling of corrupted backup logs | **Impact:** Invalid backup log block lengths are reported as corruption before processing. Valid backups and normal restore behavior are unchanged. <br> **Solution/Fix:** Improved validation of corrupted backup logs so invalid block lengths are reported as backup corruption before the data is processed. Valid backup log format and normal backup/restore behavior are unchanged. No configuration changes are required. |
| Bug fix | Correct partition keys in native Reliable Collections abort callbacks | **Impact:** Service abort callbacks receive the retrieved partition low key and are not invoked when that key cannot be obtained. <br> **Solution/Fix:** Corrected partition-key handling for native Reliable Collections service abort callbacks. The callback now receives the retrieved partition low key and is not invoked if that key cannot be obtained. No API signature or configuration changes are required. |
| Bug fix | More reliable secondary replica recovery | **Impact:** A missing secondary-build metric dimension no longer causes a metrics exception to fault the replica. <br> **Solution/Fix:** Prevents a secondary-build metrics exception from faulting a replica when the build-type dimension is unavailable, improving replica recovery and partition availability. No application or configuration changes are required. |
| Bug fix | Recovery after interrupted checkpoint-file replacement | **Impact:** Avoids a service-host crash during replica open when the current and temporary key checkpoint files are both missing. Recovery requires a healthy primary. <br> **Solution/Fix:** Improved recovery after an interrupted checkpoint-file replacement when both the current and temporary key checkpoint files are missing, avoiding a service-host crash during replica open. The affected replica returns a recoverable error so it can be rebuilt from a healthy primary. No configuration change is required; healthy replicas must be available for normal recovery. |
| Bug fix | Consistent input validation and replica-store commit timeouts | **Impact:** Invalid service-group data and inconsistent system-service metadata are rejected with errors. Replica-store commits honor the requested timeout. <br> **Solution/Fix:** Improves handling of invalid service-group operation data and inconsistent system-service metadata, and correctly propagates the requested replica-store commit timeout. No configuration change is required. Applications or tooling receiving validation errors should correct the affected input or metadata. |
| Reliability update | Safer node restarts and improved availability diagnostics | **Impact:** Affected restart deactivations may remain pending until safety checks pass. Custom diagnostic event consumers may need updates. <br> **Solution/Fix:** Applies restart safety checks to down nodes when self-reconfiguring services are enabled, and improves membership-based availability handling. Diagnostic manifests also gain high-thread-count reporting and update several event definitions. Allow affected deactivations to complete their safety checks. Review custom event decoders against the updated manifests; no safety-timeout configuration change is recommended. |
| Bug fix | Consistent partition listings during rapid repartitioning | **Impact:** Prevents rapid partition add/remove operations from leaving an otherwise healthy partition absent from service partition-list queries. <br> **Solution/Fix:** Fixes a concurrency issue in rapid add/remove repartition operations that could leave the service partition-list query persistently incomplete. No configuration change is required. Contact support if a previously incomplete partition listing persists after upgrade. |
| Bug fix | Improved transport stability during message-buffer sharing | **Impact:** Reduces the risk of process failures during concurrent sharing of transport message buffers. <br> **Solution/Fix:** Corrected concurrency handling for shared transport message buffers. Message formats and application APIs are unchanged. No application or configuration changes are required. |
| Bug fix | More reliable federation arbitration during shutdown | **Impact:** Reduces the risk of process failures when arbitration work is still pending as a vote proxy closes. <br> **Solution/Fix:** Pending arbitration callbacks and cleanup work now handle proxy closure safely. Arbitration behavior for active proxies is unchanged. No application or configuration changes are required. |
| Bug fix | More reliable TCP transport shutdown | **Impact:** Shutdown waits for outstanding receive and cleanup callbacks and avoids a cleanup-related deadlock. <br> **Solution/Fix:** Transport resources are retained until outstanding callbacks drain. A stalled drain produces diagnostics and is bounded by a final process-termination safeguard. No configuration change is required. Investigate persistent callback-drain warnings through normal support channels. |
| Bug fix | Consistent handling of incomplete federation messages | **Impact:** Partially populated edge-probe messages use defined defaults. Fully populated messages and the message format are unchanged. <br> **Solution/Fix:** Makes handling of partially populated edge-probe messages deterministic while preserving the existing message format. No configuration change is required. |
| Feature | Aligned primary moves for single-replica stateful services | **Impact:** Eligible parent/child affinity groups with one replica can participate in synchronized primary moves while preserving alignment. <br> **Solution/Fix:** Extends synchronized primary movement to single-replica stateful partitions with aligned affinity, helping preserve parent/child alignment during supported moves. No new configuration is required by this change; validate existing synchronized-movement workflows during upgrade testing. |
| Bug fix | More consistent parent/child service placement | **Impact:** Long-standing extra replicas no longer cause affinity to be relaxed for unrelated replica transitions. <br> **Solution/Fix:** Corrects aligned-affinity placement when a partition has long-standing extra replicas, so affinity is relaxed for an actual pending primary swap rather than any replica transition. No configuration change is required. |
| Bug fix | Correct placement state after failover when the correction is enabled | **Impact:** Applies only when the existing `EnableROSSnapshotServiceInfoWiring` switch is enabled. It remains disabled by default. <br> **Solution/Fix:** Corrected the service metadata used to initialize placement state after failover, improving handling of down replicas, disabled services, and replica-set sizes. This release does not enable the correction by default. No new configuration change is required by this release note. |
| Bug fix | Fewer unnecessary balancing operations for custom metrics | **Impact:** Improves balance evaluation when some nodes have zero load and reduces evaluation work for large service populations. <br> **Solution/Fix:** Improves balance evaluation for custom metrics when some nodes have zero load, avoiding unnecessary balancing and reducing evaluation work in large service populations. |
| Bug fix | More accurate placement-service health warnings | **Impact:** Reduces premature or stale unavailability warnings during transient failover while retaining warnings for the unavailable error state. <br> **Solution/Fix:** Corrected placement-service unavailability health reporting so the warning is emitted only when the placement dispatcher enters the unavailable error state. No configuration changes are required. |
| Bug fix | Reliable Collections recovery for older .NET Framework applications | **Impact:** Avoids loading an incompatible implementation assembly when persisted state-provider type names contain versionless `Microsoft.ServiceFabric.Data.Impl` references. <br> **Solution/Fix:** Fixes recovery for older .NET Framework applications whose persisted state-provider type names contain versionless `Microsoft.ServiceFabric.Data.Impl` assembly references, preventing an incompatible implementation assembly from being selected. No application configuration or persisted-data changes are required. |
| Bug fix | Safer handling of cluster-upgrade validation timeouts | **Impact:** Late completion of a timed-out download or validation operation no longer terminates the node process. <br> **Solution/Fix:** Improves reliability when cluster-upgrade validation times out, preventing a late completion from terminating the node process. No configuration change is required. |
| Bug fix | Improved validation of file-share backup locations | **Impact:** Restore requests with invalid backup locations are rejected. Retention cleanup skips invalid deletion targets and continues processing valid backups. <br> **Solution/Fix:** Improved Backup and Restore Service validation for file-share backup locations. No configuration changes are required. Applications should use backup locations returned by Service Fabric backup enumeration. |
| Reliability update | Backup and restore support for `Premium_LRS` `BlockBlobStorage` accounts | **Impact:** Fixes backup, enumeration, validation, and restore failures with `Premium_LRS` `BlockBlobStorage` accounts that have no Azure Table endpoint. <br> **Solution/Fix:** Backup and Restore Service uses blob-based backup metadata without attempting to initialize Azure Table storage for these accounts. `FlagUseAzureTableForBackupMetadata` also supports disabling Azure Tables for backup metadata. No new configuration is required. |

## Retirement and Upgrade Callouts

* Service Fabric plans to discontinue support for the Java SDK on 03/31/2027. Applications using the Java SDK will continue to run; plan migration to the Service Fabric .NET SDK.

* Before upgrading Linux clusters to Service Fabric 11.9, validate and correct all explicitly configured TLS 1.3 cipher suites, or remove the explicit cipher suite configuration to use the system defaults.

* Service Fabric plans to archive runtime versions earlier than 7.2 and SDK versions 4.2 and earlier. Use supported runtime and SDK versions to avoid scaling, reimaging, or rollback disruption.

## Repositories and Download Links

* [Service Fabric documentation](https://learn.microsoft.com/azure/service-fabric/)
* [Service Fabric samples and source repositories](https://github.com/microsoft/service-fabric)
* [Service Fabric release information](https://learn.microsoft.com/azure/service-fabric/release-notes)

### Service Fabric 11.9 Repositories and Download Links

Runtime: https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.9.121.1.exe

SDK: https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.9.121.msi

Cab: https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/11.9.121.1/MicrosoftServiceFabric.11.9.121.1.cab

Package: https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.9.121.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.9.121.1.zip

Goalstate: https://download.microsoft.com/download/7/d/1/7d1d1511-59a4-4933-8187-40c20065aa29/11.9.121.1/goalstate.11.9.121.1.json


