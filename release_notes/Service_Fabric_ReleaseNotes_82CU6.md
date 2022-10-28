# Microsoft Azure Service Fabric 8.2 Cumulative Update 6.0 Release Notes

* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Breaking Changes](#breaking-changes)
* [Key Announcements](#key-announcements)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions
The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Ubuntu 20 <br> Windows | 8.2.1485.1 <br> 8.2.1485.1 <br> 8.2.1686.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 8.2.1686.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 5.2.1686  <br> 8.2.1686 <br> 8.2.1686 <br> 8.2.1686 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15  <br> 11.0.1 |

## Breaking Changes
Customers running Azure Service Fabric Linux Clusters will not be able to scale out or reimage (manual or auto via VMSS OS Image Update) nodes or create new Linux clusters without first upgrading to Service Fabric version 9.0.1086.1. Also, if your application requires JDK, then you will need to install a JDK of your choice since SF will not install the JDK as part of the SF installation.
Starting 8/19/2022, the following versions have been deprecated and will not be available on Linux for new cluster creation or node reimage/scale out operations:<br>
* 9.0 CU2/CU1/RTO
* 8.2 CU4/CU3/CU2.1/CU2/CU1/RTO<br>

**Details:** Service Fabric Runtime (for Linux) placed a dependency on Zulu for Azure build of OpenJDK which is no longer supported as announced in their dev [blog](https://devblogs.microsoft.com/java/end-of-updates-support-and-availability-of-zulu-for-azure/). Since the creation of new Linux clusters or scale-out/reimage of nodes required all the dependencies to be installed, these workflows broke when the dependency became unavailable.<br>
Beginning with 8.2 CU 5.1 release, Service Fabric Runtime will no longer install any JDK dependency which will ensure that customer can scale out, reimage, or create new Linux clusters. If your application needs JDK, please utilize an alternate mechanism to provision a JDK. To get the latest Service Fabric Runtime releases with this change, follow the upgrade documentation. If you depend on a JDK, please be sure to install one of your choice.<br>
For more information see: [Breaking change for Azure Service Fabric Linux customers](https://techcommunity.microsoft.com/t5/azure-service-fabric-blog/breaking-change-for-azure-service-fabric-linux-customers/ba-p/3604678)

**Breaking Changes with  BackupRestoreService:**
If an SF cluster has periodic backup enabled on any of the app/service/partition, post upgrade to 8.2.1686.9590 even though user app, cluster and BRS shows healthy. BRS will fail to deserialize old BackupMetadata and stop taking backup and restore on the partition/service/app in question  with changes in new release.

**Identifying the issue:**
There are two ways to identify and confirm the issue

A. If periodic backups were happening on any partition, it should be visible on SFX under Cluster->Application->Service->Partition->Backup. Here list of all backups being taken with creation time is available. Using this info and upgrade time, customer can identify whether a backup policy was enabled, backups were happening before upgrade and whether backups are happening post upgrade.

B. Another way of checking and enumerating backups is calling this API [get partition backup list](https://learn.microsoft.com/en-us/rest/api/servicefabric/sfclient-api-getpartitionbackuplist).

**Mitigation:**
To mitigate, customers need to update the existing policy after upgrading to 8.2.1686.9590. User can call Update API for Backup Policy as mentioned in this doc [Update Backup Policy](https://learn.microsoft.com/en-us/rest/api/servicefabric/sfclient-api-updatebackuppolicy) with existing policy values. It will update the policy model inside BRS with new data model and BRS will start taking periodic backups again.

**Steps:**
1. Check and confirm issue as mentioned in "Identifying the issue" section above.
2. If the issue is confirmed, update the backup policy with same old values by calling Update API for Backup Policy. Below is one sample -
    ```powershell
     $BackupPolicy=@{
      Name = "DailyAzureBackupPolicy"
      AutoRestoreOnDataLoss = "false"
      MaxIncrementalBackups = "3"
      Schedule = @{
        ScheduleKind = "FrequencyBased"
        Interval = "PT3M"
      }
      Storage = @{
        StorageKind = "AzureBlobStore"
        FriendlyName = "Azure_Storage_Sample"
        ConnectionString = "<connection string values>"
        ContainerName = "<Container Name>"
      }
      RetentionPolicy = @{
        RetentionPolicyType = "Basic"
        MinimumNumberOfBackups = "20"
        RetentionDuration = "P3M"
      }
     }
      $body = (ConvertTo-Json $BackupPolicy)
      $url = 'https://<ClusterEndPoint>:19080/BackupRestore/BackupPolicies/DailyAzureBackupPolicy/$/Update?api-version=6.4'
      Invoke-WebRequest -Uri $url -Method Post -Body $body -ContentType 'application/json' -CertificateThumbprint '<Thumbprint>'
      # User should update the name of backup policy [DailyAzureBackupPolicy being used here and other possible values accordingly].
    ```
3. Wait for 1-2 mins and policy should get updated across all entities.
4. Periodic backups will start happening as per backup policy and it can be confirmed by enumerating them.


## Key Announcements
* Azure Service Fabric will block deployments that do not meet Silver or Gold durability requirements starting on 11/10/2022. 5 VMs or more will be enforced with this change for newer clusters created after *11/10/2022* to help avoid data loss from VM-level infrastructure requests for production workloads. VM count requirement is not changing for Bronze durability. Enforcement for existing clusters will be rolled out in the coming months. <br> For details see: [Durability characteristics of the cluster](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-capacity#durability-characteristics-of-the-cluster). 
* Azure Service Fabric node types with VMSS durability of Silver or Gold should always have the property  "virtualMachineProfile.osProfile.windowsConfiguration.enableAutomaticUpdates" set to false in the scale set model definition. Setting this will prevent unintended OS restarts due to the Windows updates like patching, which can impact the production workloads. Instead you should enable Automatic OS upgrades through VMSS OS Image updates by setting "enableAutomaticOSUpgrade" to true. With automatic OS image upgrades enabled on your scale set, an extra patching process through Windows Update is not required.<br>
For more information see: [VMSS Image Upgrades](https://docs.microsoft.com/en-us/azure/virtual-machine-scale-sets/virtual-machine-scale-sets-automatic-upgrade)

## Service Fabric Feature and Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows - 8.2.1686.9590<br>Ubuntu 18 - 8.2.1485.1<br>Ubuntu 20 - 8.2.1485.1** | **Feature** | Backup Restore Service | **Brief Description**: BackupRestoreService (BRS) in Service Fabric is updated with new events for all backup and restore operations. This feature adds events for each intermediate state during backup and restore operations at partition level. BRS will also fire periodic events about backup and policies at partition and cluster level. These events can be viewed on Service Fabric Explorer.<br> **Documentation Reference**: Read more about it at - [BackupRestoreService events](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-diagnostics-event-generation-operational) 

## Retirement and Deprecation Path Callouts
* Service Fabric ASP.NET Core packages built against ASP.NET Core 1.0 binaries are out of support. Starting Service Fabric 8.2, we will be building Service Fabric ASP.NET Core packages for .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, .NET Framework 4.6.1. As a result, they can be used only to build services targeting .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, >=.NET Framework 4.6.1 respectively.
For .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, Service Fabric ASP.NET Core will be taking dependency on Microsoft.AspNetCore.App shared framework; whereas, for NetFx target frameworks >=.NET Framework 4.6.1, Service Fabric ASP.NET Core will be taking dependency on ASP.NET Core 2.1 packages.
The package Microsoft.ServiceFabric.AspNetCore.WebListener will no longer be shipped, Microsoft.ServiceFabric.AspNetCore.HttpSys package should be used instead.
* .NET 5.0 runtime has reached end-of-life on May 8, 2022. Service Fabric releases after this date will drop support for Service Fabric applications running with .NET 5.0 runtime. Current applications running on .NET 5.0 runtime will continue to work, but requests for investigations or requests for changes will no longer be entertained. Please migrate to using .NET 6.0 version instead.
* Ubuntu 16.04 LTS reached its 5-year end-of-life window on April 30, 2021. Service Fabric runtime has dropped support for 16.04 LTS, and we recommend moving your clusters and applications to Ubuntu 18.04 LTS or 20.04 LTS. Current applications running on it will continue to work, but requests to investigate issues w.r.t this OS or requests for change will no longer be entertained. Service Fabric runtime has also dropped producing builds for U16.04 so customers will no longer be able to get SF related patches.
* Service Fabric runtime will soon stop supporting BinaryFormatter based remoting exception serialization by default and move to using Data Contract Serialization (DCS) based remoting exception serialization by default. Current applications using it will continue to work as-is, but Service Fabric strongly recommends moving to using Data Contract Serialization (DCS) based remoting exception instead.
* Service Fabric runtime will soon be archiving and removing Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 8.2.1485.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 8.2.1686.9590 | N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.8.2.1686.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package | 8.2.1686.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/8.2.1686.9590/Microsoft.Azure.ServiceFabric.WindowsServer.8.2.1686.9590.zip |
||Service Fabric Standalone Runtime | 8.2.1686.9590 |N/A | https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/8.2.1686.9590/MicrosoftAzureServiceFabric.8.2.1686.9590.cab |
|.NET SDK |Windows .NET SDK | 5.2.1686 |N/A | https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.5.2.1686.msi  |
||Microsoft.ServiceFabric | 8.2.1686 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf | 8.2.1686 |https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*| 8.2.1686 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 8.2.1686 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 8.2.1686 | N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.6 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.6 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator | 1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 11.1.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |N/A  |
