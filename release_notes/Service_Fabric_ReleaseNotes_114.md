# Microsoft Azure Service Fabric 11 CU4 (Auto Upgrade) Release Notes

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
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.4.205.1.exe) | Ubuntu 22 <br> Windows | 11.4.205.4 <br> 11.4.205.1 |
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.4.205.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.4.205.1.zip) | Service Fabric Standalone Installer Package | 11.4.205.1 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.4.205.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 8.0.0 <br> 11.4.205.1 <br> 8.0.0 <br> 8.0.0 |
| [Java SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.4.205.msi) | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module<br>SFCTL | 0.3.15 <br> 11.0.1 |

## Service Fabric Feature and Bug Fixes

| **Versions** | **Issue Type** | **Description** | **Resolution** |
|---|---|---|---|
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | PrimaryNodeName variable | **Brief Description:**  This release (11CU4) introduces significant improvements to the Infrastructure Service to properly handle stale documents during primary failover scenarios. <br> **Feature/Bug Impact:** When you specify a "PrimaryNodeName", PLB will check the current location of the partition's primary replica to see if it matches the node specified. If the "PrimaryNodeName" is different from that of the current primary replica's location, it will ignore the primary load update. This ensures we do not update a newly moved/swapped primary's metrics with that of an old primary replica. <br> **Solution/Fix:** "PrimaryNodeName" was added to the "PartitionMetricLoadDescription" class, and added to the corresponding API call "FABRIC_PARTITION_METRIC_LOAD_DESCRIPTION". <br> **Documentation Reference:** [REST](https://learn.microsoft.com/rest/api/servicefabric/sfclient-model-partitionmetricloaddescription), [.NET](https://learn.microsoft.com/previous-versions/dotnet/api/system.fabric.description.partitionmetricloaddescription?view=azure-dotnet-archive-legacy), [Python](https://learn.microsoft.com/python/api/azure-servicefabric/azure.servicefabric.models.partitionmetricloaddescription?view=azure-python) <br> **Breaking Change:** Using this extension requires using SF version 11.3 or 10.1CU10. If ResourceOrchestrator is enabled, ResourceOrchestratorService will also need to be on 11.3/10.1CU10. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | Infrastructure Service | **Brief Description:** The replica soft delete feature is an enhancement to the remove replica API workflow, where we prevent users from accidentally losing data from their stateful persisted service, and allows them to recover their data. The need stems from various incidents including Azure outages caused by the misuse of this data destructive operation. <br> **Feature/Bug Impact:** Infrastructure Service now avoids cancelling an active repair task when a stale document is received during a primary failover. Previously, such tasks could be cancelled incorrectly. This change is backward compatible. <br> **Solution/Fix:** The repair task cancellation logic has been updated to use both the document incarnation number and the MR conversation ID, preventing incorrect cancellation of active repair tasks. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | Reliable Collections (RC) Data | **Brief Description:** Tooling that allows reading and manipulating on-disk RC data. <br> **Feature/Bug Impact:** Allows fixing critical issues in production such as data corruption and data loss by manipulating live RCs. <br> **Solution/Fix:** Standalone tool that can be used to read and edit RC data. Partition can then be restored from the modified data. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | Reliable Collections Checkpoint Merges | **Brief Description:** Improvements for checkpoint merges for reliable collections. This includes retry logic if a checkpoint file selected for merge has value corruption which would previously prevent all merges from succeeding resulting in disk bloat. Now the merge is reattempted without the corrupt file so only that single file will get leaked until cleanup and prevents disk bloat. When selecting the files to merge we now cap the number of files to a configurable value defaulted to 32. This is to prevent situations when there are a large amount of files to merge which extends the merge time to be extremely long due to the need to deduplicate the logic found within. <br> **Feature/Bug Impact:** Improvements to merge to prevent single file corruption from impacting all merges even pertaining to clean files. Add a cap to number of files to merge to prevent large amount of files from getting merging at one time and instead allow for incremental decreases in number of files. <br> **Solution/Fix:** These features are on by default. **Workaround:** The cap for number of files can be adjusted via the MaxFilesToMerge setting. This can be set to a large number if you do not want to take advantage of this feature. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | Health Events | **Brief Description:** This release (11CU4) introduces health events for jobs that are being throttled by the Infrastructure Service and provides visibility into the reasons for the throttling. <br> **Feature/Bug Impact:** Customers and the SF DRI will be able to clearly understand the exact reason why a job is being throttled and make informed decisions based on that information. This change is backward compatible. <br> **Solution/Fix:** Health events are emitted after collecting detailed information about the job and the specific reasons it is being throttled. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | Health Events | **Brief Description:** This release (11CU4) introduces warning health events for the scenario in which Infrastructure service is missing from the cluster for a zone. <br> **Feature/Bug Impact:** This health event provides customers with visibility when the Infrastructure Service is missing from a cluster. It helps explain why cluster updates are stalled and not being approved by Service Fabric. This change is backward compatible. <br> **Solution/Fix:** This health event alerts customers to scenarios where the Infrastructure Service is missing for a zone in the cluster. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | WCF Remoting Binary Serialization Deprecation | **Brief Description:** Binary serialization for WCF Remoting is deprecated and will be removed in Service Fabric SDK release 9.0. Release 8.1.0 enables migration of services from binary serialization to DCS (Data Contract Serialization). <br> **Feature/Bug Impact:** Binary serialization for WCF Remoting will be removed in Service Fabric SDK release 9.0build time. <br> **Solution/Fix:** Customers can upgrade their Service Fabric SDK to release 8.1.0 which enables seamless migration from binary serialization to DCS for WCF Remoting. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | Native TStore | **Brief Description:** Enables Native TStore to track each entry’s LastModifiedUTC timestamp and expose it through TSReplicatedStore ReadExact and Enumeration, aligning its behavior with ESE. <br> **Feature/Bug Impact:** With TrackLastModifiedUTC enabled, customers gain access to LastModifiedUTC metadata for each store entry. Behavior will match ESE except that the timestamp reflects the primary replica’s value regardless of whether reads occur on the primary or a secondary. <br> **Solution/Fix:** Native TStore can now track LastModifiedUTC and persist the data to disk. Thus key checkpoint files, along with the write-ahead-log track this additional information per entry if config for the feature is enabled. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | CetCurrentDocsFromIS API | **Brief Description:** This release (11CU4) introduces the GetCurrentDocFromIS API, which enables retrieval of the current Infrastructure Service (IS) document directly from the Infrastructure Service. <br> **Feature/Bug Impact:** The impact of this change is that the current Infrastructure Service document can now be retrieved using this API. This change is backward compatible. <br> **Solution/Fix:** The GetCurrentDocFromIS API retrieves the latest Infrastructure Service document as its response. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Feature | Types Deprecations | **Brief Description:** All types in the following namespaces have been deprecated. Migration of Actor state will be done as part of the general data migration between Service Fabric data stores: <br> - *Microsoft.ServiceFabric.Actors.Migration* <br> - *Microsoft.ServiceFabric.Actors.Runtime.Migration* <br> **Feature/Bug Impact:** Any code referencing deprecated types will emit C# warning 618 at build time. <br> **Solution/Fix:** Customers should stop using deprecated types and plan for their removal in the next major version of the Service Fabric SDK. |
| Windows - <br> 11.4.205.1 <br> Ubuntu 22 - <br> 11.4.205.4 | Bug | Autoscaling Breakage on Upgrade | **Brief Description:** : When upgrading from SF 10.x to 11.x autoscaling will stop working on services that have it set up. <br> **Feature/Bug Impact:** Autoscaling stops working due to a bad initialization of new internal structures during the upgrade. <br> **Solution/Fix:** The fix improves the upgrade flow by making sure the new structures are always initialized with correct data. **Workaround:** A working mitigation is to update the scaling policy on the service, which will trigger the working initialization flow. After the update, the scaling policy can again be updated back to its initial state, meaning that the initial update must be a real change for the mitigation to work. The mitigation can be performed using the Update-ServiceFabricService command. |

## Retirement and Deprecation Path Callouts

* **Important:** Certificate Authorities are removing ClientAuth EKU from publicly trusted TLS certificates between May and October 2026. Service Fabric clusters using publicly issued certificates must ensure their cluster and client certificates retain client EKU capability to avoid service disruptions. For detailed information and action items, please see the [Client EKU Removal Addendum](Resources/ClientEKURemoval.md).

* Service Fabric (SF) runtime will discontinue support for the Java SDK soon. For a smooth transition, we strongly recommend users to shift to Azure Service Fabric .NET SDK. If your current setup is based on the Service Fabric Java SDK, we suggest starting migration plans to smoothly switch to the Azure Service Fabric .NET SDK. Although applications using the Java SDK will continue to work, we highly recommend adopting the SF .NET SDK for optimal outcomes.

* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.
  * To prevent disruption of workloads, create a new cluster using the following steps:
    * [Create a Service Fabric cluster using ARM template](/azure/service-fabric/quickstart-cluster-template)
    * [Create a Standalone cluster](/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)
    * Install the supported version of Service Fabric SDK based on the Runtime version installed on the cluster.

* Starting in Service Fabric 11 CU3, support for stateless instance removal will begin its deprecation pathway. Support for stateless instance removal will be fully removed approximately one year after the release of 11 CU3.
  * The following cmdlets and API calls will be deprecated for stateless replicas:
    * [Remove-ServiceFabricReplica](https://learn.microsoft.com/en-us/powershell/module/servicefabric/remove-servicefabricreplica)
    * [FabricClient.FaultManagementClient.RemoveReplicaAsync](https://learn.microsoft.com/en-us/dotnet/api/system.fabric.fabricclient.faultmanagementclient.removereplicaasync)
    * [Remove Replica – Microsoft Learn](https://learn.microsoft.com/en-us/rest/api/servicefabric/sfclient-api-removereplica)
  * Support for stateless instance removal will be fully removed approximately one year after the release of 11 CU3:
  * **Customer impact:** Using these APIs after upgrading to version 11 CU3 will trigger deprecation warnings. Full removal of support is targeted for a future release, approximately one year post-11.3.
    * Scripts and tools depending on the deprecated APIs may fail in future versions.
    * Customers should begin migrating to the new APIs as soon as they become available to prevent disruptions in automation workflows.
    * Alternate APIs to support stateless instance removal will be announced in a 11 CU3 release.
  * **Migration path:** Service Fabric 11 CU3 introduces new APIs and a compatibility switch to ensure existing scripts continue functioning after upgrading, while allowing time to transition.
    * Full details about replacements and migration guidance will be included in the 11 CU3 release notes and documentation.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

Runtime:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.4.205.1.exe

SDK:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.4.205.msi

Cab:
https://download.microsoft.com/download/b/0/b/b0bccac5-65aa-4be3-ab13-d5ff5890f4b5/11.4.205.1/MicrosoftServiceFabric.11.4.205.1.cab

Package:
https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.4.205.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.4.205.1.zip
 
Goalstate:
https://download.microsoft.com/download/7/d/1/7d1d1511-59a4-4933-8187-40c20065aa29/11.4.205.1/goalstate.11.4.205.1.json
