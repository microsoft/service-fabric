# Microsoft Azure Service Fabric 10.1 Cumulative Update 7.0 Release Notes

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
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.10.1.2841.9590.exe) | Ubuntu 20 <br> Ubuntu 22 <br> Windows | 10.1.2844.9590 <br> 10.1.2846.9590 <br> 10.1.2841.9590  |
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/10.1.2841.9590/Microsoft.Azure.ServiceFabric.WindowsServer.10.1.2841.9590.zip) | Service Fabric Standalone Installer Package | 10.1.2841.9590 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.1.2841.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 7.1.2175 <br> 10.1.2175 <br> 7.1.2175 <br> 7.1.2175 |
| [Java SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.1.2841.msi) | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module<br>SFCTL | 0.3.15 <br> 11.0.1 |

## Service Fabric Feature and Bug Fixes

| **Versions** | **Issue Type** | **Description** | **Resolution** |
|---|---|---|---|
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590 | Bug | App Config Only Upgrade | **Brief Description:** Changed the behavior for app config only upgrade with force restart. Previous when a config only app upgrade with force restart, in the rolling forward, replicas are restarted, when rolling forward failed, in the rollback, the replicas that haven't been upgraded in the rolling forward got restarted. from this service fabric release, during the rolling back, the replicas that haven't been upgraded in the rolling forward will NOT restart. <br> **Feature/Bug Impact:** The replicas that didn't upgraded during the rolling forward will NOT restart anymore in the rollback after the app config only upgrade failed. <br> **Solution/Fix:** The fix updates the logic in app upgrade force-restart mode. In the rolling forward or rollback, only restart the replicas that the current replica package content checksum is different from target package content checksum. the un-upgraded replicas, even though the rolling version is different, however, both package content checksum is the same. <br> **Breaking Change:** During application config only upgrade with force restart, replica will not restart during the rollback if it hasn't been upgraded in the rolling forward. <br> **Workaround:** If during the app config only upgrade, replica needs to restart during rolling forward, however, hit some external issue, the app upgrades start to rollback, and don't want to restart the replicas that haven't been upgraded. you could update the upgrade mode to Unmonitored Manual, after the external issue got fixed. update the upgrade mode back to Monitored. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590 | Bug | Central Secret Service Throttling Mechanism | **Brief Description:** This change introduces a throttling mechanism in the Central Secret Service. This feature is designed to prevent the recurrence of situation where parallel app activations can cause CSS to do unbounded work, slowing down and causing app activations to fail. <br> **Feature/Bug Impact:** Parallel app activations, each with numerous secrets, overload the Central Secret Service. Because of this, no request is processed on time, which leads to all of them timing out. The final outcome is that no application can start because this cycle repeats. <br> **Solution/Fix:** With the throttling implementation, we have the ability to avoid the mentioned situation easily by enabling throttling, ensuring that all applications will be up at some point and CSS won't be overloaded at any moment. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590 | Feature | CM Query | **Brief Description:** Optimize CM query Get-ServiceFabricApplication. <br> **Feature/Bug Impact:** When there are a lot of applications in cluster and CM is busy and experiencing high memory usage, Get-ServiceFabricApplication can take longer operation time and more memory. <br> **Solution/Fix**: The fix is to optimize CM query logic. Instead of reads whole "db"(cm store), it only reads necessary data from the "db" to reduce CM unnecessary read. <br> **Workaround:** Increase the value of parameter timeout in query request, then the query request has more time to complete the request. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590 | Bug | Reliable Dictionaries | **Brief Description:** Reliable Dictionaries, when encountering the error for "Attempting to open an already log file, which is not allowed," will perform the mitigation step of forcefully removing the replica automatically instead of allowing the replica to infinitely cycle until manual intervention is provided. <br> **Feature/Bug Impact:** Replicas based on Reliable Dictionaries will now automatically force remove themselves when encountering this error prompting the primary to create a new healthy replica and preventing an infinite cycling of the replica. <br> **Solution/Fix:** When encountering this error Reliable Dictionaries will utilize the Fabric Client to automatically force remove the affected replica to allow creation of a new replica and bring the partition back to a healthy state automatically. <br> **Workaround:** If this error message is hit as shown in an SFX warning health event, the replica can be forcibly removed by using the Remove-ServiceFabricReplica powershell cmdlet. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590 | Feature | Reliable Dictionaries | **Brief Description:** Reliable Dictionaries now prevent the reuse of bad file streams when reading from Value Checkpoint Files and will restart the replica in the event that a value cannot be read due to repeated attempts to use bad file streams. <br> **Feature/Bug Impact:** Reliable Dictionaries will dispose of bad file streams instead of adding back to the pool to prevent reusing of these file streams and preventing repeated failed reads due to a possible issue with a single stream. <br> **Solution/Fix:** When encountering an exception associated with a bad file stream the stream will be disposed and the read will be attempted with a new file stream. If after 3 attempts the value still cannot be read due to file stream related issues, the replica will be restarted automatically to mitigate the issue when there is quorum. <br> **Workaround:** If Reliable Dictionary reads repeatedly fail with ObjectDisposedExceptions, the replica can be restarted manually to mitigate the issue. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590 | Feature | Reliable Dictionaries | **Brief Description:** Service Fabric Runtime for Linux updated its .NET dependency from 6 to 8. This causes .NET 8 to be installed on the system instead of .NET 6. <br> **Feature/Bug Impact:** This makes it so Service Fabric for Linux continues to use a support .NET. <br> **Solution/Fix:** Updates the dependencies that Service Fabric for Linux has on installing its DEB package to specify .NET 8 packages instead of .NET 6 packages. <br> **Breaking Change:** It is possible this will break customers who are not installing .Net 6 to support their own applications. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590 | Bug | ManagedIdentityTokenService | **Brief Description:** In this release we fixed a bug where ManagedIdentityTokenService could fail to update its certificate, causing managed identity token requests to fail unexpectedly. With this update, ManagedIdentityTokenService ensures that it always uses the most up-to-date certificate. <br> **Feature/Bug Impact:** ManagedIdentityTokenService requests could sometimes fail unexpectedly. <br> **Solution/Fix:** HttpGatewayClient is instantiated with the up-to-date certificate every time when communication with FabricGateway is initiated. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590  | Feature | Backup Restore System (BRS) | **Brief Description:** The Backup Restore Service provides robust and reliable mechanisms for backing up and restoring stateful services. It ensures data durability and availability by allowing users to create backups of their service states and restore them in the event of failures or during routine maintenance. This service is vital for maintaining business continuity and minimizing downtime. <br> <br> **Increased RestoreTimeout:** We have increased the default RestoreTimeout from 10 minutes to 30 minutes to allow more time for the completion of restore operations <br> - New Default RestoreTimeout: 30 minutes <br> - This change will help prevent multiple restore attempts and ensure a smoother restore process for users. <br> <br> **Benefit:** Improved efficiency and fewer timeouts with the increased RestoreTimeout. |
| Windows - <br> 10.1.2841.9590 <br> Ubuntu 20 - <br> 10.1.2844.9590 <br> Ubuntu 22 - <br> 10.1.2846.9590  | Feature | Backup Restore System (BRS) | **Brief Description:** The Backup Restore Service provides robust and reliable mechanisms for backing up and restoring stateful services. It ensures data durability and availability by allowing users to create backups of their service states and restore them in the event of failures or during routine maintenance. This service is vital for maintaining business continuity and minimizing downtime. <br> <br> **SecondariesInProgress**: We have introduced a new state called SecondariesInProgress in the restore progress. This state helps to provide more precise information about the progress of the restore operation: <br> - SecondariesInProgress: This new state indicates that the primary replica has been successfully restored, and the restoration of secondary replicas is currently in progress (Partition’s quorum not recovered yet). <br> <br> **Benefit:** Enhanced transparency in the restore process with the new SecondaryInProgress state. |

## Retirement and Deprecation Path Callouts

* Service Fabric (SF) runtime will discontinue support for the Java SDK soon. For a smooth transition, we strongly recommend users to shift to Azure Service Fabric .NET SDK. If your current setup is based on the Service Fabric Java SDK, we suggest starting migration plans to smoothly switch to the Azure Service Fabric .NET SDK. Although applications using the Java SDK will continue to work, we highly recommend adopting the SF .NET SDK for optimal outcomes.

* Ubuntu 18.04 LTS reached its 5-year end-of-life window on June-2023. Service Fabric runtime has dropped support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](/azure/service-fabric/service-fabric-versions)

* Previously communicated, Service Fabric runtime had planned to remove Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older, from the package Download Center in July 2023. We would like to inform you that this timeline has been extended, and the removal will now take place in January 2024.

* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.
  * To prevent disruption of workloads, create a new cluster using the following steps:
    * [Create a Service Fabric cluster using ARM template](/azure/service-fabric/quickstart-cluster-template)
    * [Create a Standalone cluster](/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)
    * Install the supported version of Service Fabric SDK based on the Runtime version installed on the cluster.

* Starting in Service Fabric 11.2, support for stateless instance removal will begin its deprecation pathway. Support for stateless instance removal will be fully removed approximately one year after the release of 11.2.
  * The following cmdlets and API calls will be deprecated for stateless replicas:
    * [Remove-ServiceFabricReplica](https://learn.microsoft.com/en-us/powershell/module/servicefabric/remove-servicefabricreplica)
    * [FabricClient.FaultManagementClient.RemoveReplicaAsync](https://learn.microsoft.com/en-us/dotnet/api/system.fabric.fabricclient.faultmanagementclient.removereplicaasync)
    * [Remove Replica – Microsoft Learn](https://learn.microsoft.com/en-us/rest/api/servicefabric/sfclient-api-removereplica)
  * Support for stateless instance removal will be fully removed approximately one year after the release of 11.2:
  * **Customer impact:** Using these APIs after upgrading to version 11.2 will trigger deprecation warnings. Full removal of support is targeted for a future release, approximately one year post-11.2.
    * Scripts and tools depending on the deprecated APIs may fail in future versions.
    * Customers should begin migrating to the new APIs as soon as they become available to prevent disruptions in automation workflows.
    * Alternate APIs to support stateless instance removal will be announced in a 11.2 release.
  * **Migration path:** Service Fabric 11.2 introduces new APIs and a compatibility switch to ensure existing scripts continue functioning after upgrading, while allowing time to transition.
    * Full details about replacements and migration guidance will be included in the 11.2 release notes and documentation.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

Run Time:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.10.1.2841.9590.exe

SDK:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.1.2841.msi

Cab:
https://download.microsoft.com/download/b/0/b/b0bccac5-65aa-4be3-ab13-d5ff5890f4b5/10.1.2841.9590/MicrosoftAzureServiceFabric.10.1.2841.9590.cab
 
Package:
https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/10.1.2841.9590/Microsoft.Azure.ServiceFabric.WindowsServer.10.1.2841.9590.zip
 
Goalstate:
https://download.microsoft.com/download/7/d/1/7d1d1511-59a4-4933-8187-40c20065aa29/10.1.2841.9590/goalstate.10.1.2841.9590.json
