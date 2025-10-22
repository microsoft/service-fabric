# Microsoft Azure Service Fabric 11.1 (Auto Upgrade) Release Notes

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
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.1.208.1.exe) | Ubuntu 20 ([End of Life: May 31, 2025](https://ubuntu.com/blog/ubuntu-20-04-lts-end-of-life-standard-support-is-coming-to-an-end-heres-how-to-prepare)) <br> Ubuntu 22 <br> Windows | 11.1.208.3 <br> 11.1.208.4 <br> 11.1.208.1 |
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.1.208.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.1.208.1.zip) | Service Fabric Standalone Installer Package | 11.1.208.1 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.1.208.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 8.1.208 <br> 11.1.208.1 <br> 8.1.208 <br> 8.1.208 |
| [Java SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.1.208.msi) | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module<br>SFCTL | 0.3.15 <br> 11.0.1 |

## Service Fabric Feature and Bug Fixes

| **Versions** | **Issue Type** | **Description** | **Resolution** |
|---|---|---|---|
| Windows - <br> 11.1.208.1 <br> Ubuntu 20 - <br> 11.1.208.3 <br> Ubuntu 22 - <br> 11.1.208.4 | Feature | Backup and Restore Service Azure Storage Library | **Brief Description:** Azure Storage Library used by Backup and Restore Service has been updated to latest version. This requires user to give "Storage Account Contributor" role to the Managed Identity used in Backup policy with Managed Identity backed Azure Storage. <br> **Feature/Bug Impact:** Backup policy with Managed Identity backed Azure Storage, that have Managed Identity with only "Storage Blob Data Contributor" role will fail to upload or download backups. <br> **Solution/Fix:** Add "Storage Account Contributor" role to the Managed Identity. <br> **Breaking Change:**  Backups upload and download by Backup Restore Service will fail for Backup Policy with Managed Identity backed Azure Storage that has only "Storage Blob Data Contributor" assigned. |
| Windows - <br> 11.1.208.1 <br> Ubuntu 20 - <br> 11.1.208.3 <br> Ubuntu 22 - <br> 11.1.208.4 | Bug | Data Collection Agent | **Brief Description:**  Starting on SF version 10.1CU6 (Linux version 10.1.2319.1) after updating to use latest Azure tables library, when Data Collection Agent (FabricDCA) tries to cleanup old table entities on Linux cluster, will raise the following Unhandled exception System.NotSupportedException: "Method CompareOrdinal not supported". <br> **Feature/Bug Impact:** FabricDCA will crash and restart which will generate a .NET ThreadPool.dmp which can occupy a high amount of memory. <br> **Solution/Fix:** FabricDCA will use a supported method when trying to cleanup up old table entities. |
| Windows - <br> 11.1.208.1 <br> Ubuntu 20 - <br> 11.1.208.3 <br> Ubuntu 22 - <br> 11.1.208.4 | Feature | Cluster Manager (CM) | **Brief Description:** During an application upgrade, the SF system service (CM) first triggers the ImageBuilder process to process the application package. Once this is complete, SF proceeds with the UD walk for the application upgrade. <br> In the ImageBuilder process, if the application is a containerized application, CM should retrieve the container image information either from the container registry or from the host policy label configuration. However, due to a bug, CM is unable to fetch this information from the host policy label configuration. <br> This fix enables CM to correctly retrieve container image information from the host policy label configuration. <br> **Feature/Bug Impact:** CM will attempt to fetch the container info from the container registry. If the registry is inaccessible from the cluster, CM will continue retrying until it reaches the timeout limit. Once timed out, CM catches the exception and proceeds to the next step. <br> Fetching container image information is an optimization for PLB smart placement and is not strictly required. If the retrieval fails, it is not critical, and ImageBuilder will move forward. However, the repeated retries consume time, causing the ImageBuilder process to time out, ultimately leading to the application upgrade getting stuck at the initial stage. <br> **Solution/Fix:** This fix enables CM to correctly retrieve container image information from the host policy label configuration. When the container registry is inaccessible, CM can get container information from host policy label via a cluster manifest configuration. <br> **Workaround:** Increase a configuration (MinOperationTimeout under section ClusterManager) to let ImageBuilder have more time to complete. |
| Windows - <br> 11.1.208.1 <br> Ubuntu 20 - <br> 11.1.208.3 <br> Ubuntu 22 - <br> 11.1.208.4 | Bug | FM Multiple Repair Job Requests | **Brief Description:** FM can receive multiple repair job requests for the down node, with each of them having different deactivation intent. If the first of them has the data destructive intent, the process itself will be blocked. However, if any of the subsequent requests are with the non-destructive intent, safety checks will pass, but without the effective intent update. Therefore, the repair job that has destructive intent will also get approved and executed, which will result in data loss for this node. <br> **Feature/Bug Impact:** Subsequent requests won't make the safety checks to pass, leaving the data on the node intacted. <br> **Solution/Fix:** Previous code was setting the NodeDeactivationStatus to DeactivationComplete based on the intent from the latest request, not considering the effective intent, although effective intent is the one that leads the deactivation process itself. New code uses the intent from the latest request only to check if effective intent should be updated. Every following calculation is based upon effective intent. |
| Windows - <br> 11.1.208.1 <br> Ubuntu 20 - <br> 11.1.208.3 <br> Ubuntu 22 - <br> 11.1.208.4 | Bug | Downgrading | **Brief Description:** Improve capability of SF handling .NET Framework downgrade bug from .NET Core 5+ when handling types that include a custom type and a type impacted by the bug. <br> **Feature/Bug Impact:**  SF can now handle custom types that include a custom type that requires assembly information and a built in type that uses System.Private.CoreLib which is affected by a .NET bug when downgrading. <br> **Solution/Fix:** SF will now attempt to only remove assembly information from System.Private.CoreLib rather than all assembly information if previous attempts to resolve the type fail. <br> **Workaround:** N/A, customers weren't able to downgrade if they use types that include System.Private.CoreLib as well as a custom assembly. |
| Windows - <br> 11.1.208.1 <br> Ubuntu 20 - <br> 11.1.208.3 <br> Ubuntu 22 - <br> 11.1.208.4 | Feature | Token Revocation for Service Fabric Application Managed Identity | **Brief Description:** This change introduces support for Token Revocation for Service Fabric Application Managed Identity. In the case a managed identity token is revoked, a flow will be initiated to fetch a new valid managed identity token. If you are using the Azure.Identity library, this support is automatic in 4.70.2. <br> **Feature/Bug Impact:**  If the token is revoked, client can inform MITS of the revocation through the proper MSAL API. <br> **Solution/Fix:** This fix allows MITS to be aware of the client parameters that enable this scenario. Those parameters are 'xms_cc' - specifying the client capabilities and 'token_sha256_to_refresh'. While it may be possible for clients to consume this flow through their own implementation, it is highly recommended to consume the Azure.Identity library to get this and future improvements automatically. <br> **Documentation Reference**: <br> -[MSAL for .NET GitHub Changelog](https://github.com/AzureAD/microsoft-authentication-library-for-dotnet/blob/main/CHANGELOG.md) <br> -[Use managed identity with an application](https://learn.microsoft.com/en-us/azure/service-fabric/how-to-managed-identity-service-fabric-app-code) |

## Retirement and Deprecation Path Callouts

* Service Fabric (SF) runtime will discontinue support for the Java SDK soon. For a smooth transition, we strongly recommend users to shift to Azure Service Fabric .NET SDK. If your current setup is based on the Service Fabric Java SDK, we suggest starting migration plans to smoothly switch to the Azure Service Fabric .NET SDK. Although applications using the Java SDK will continue to work, we highly recommend adopting the SF .NET SDK for optimal outcomes.

* Ubuntu 18.04 LTS reached its 5-year end-of-life window on June-2023. Service Fabric runtime has dropped support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](/azure/service-fabric/service-fabric-versions)

* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors.
  * To prevent disruption of workloads, create a new cluster using the following steps:
    * [Create a Service Fabric cluster using ARM template](/azure/service-fabric/quickstart-cluster-template)
    * [Create a Standalone cluster](/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)
    * Install the supported version of Service Fabric SDK based on the Runtime version installed on the cluster.

* Starting in Service Fabric 11.3, support for stateless instance removal will begin its deprecation pathway. Support for stateless instance removal will be fully removed approximately one year after the release of 11.3.
  * The following cmdlets and API calls will be deprecated for stateless replicas:
    * [Remove-ServiceFabricReplica](https://learn.microsoft.com/en-us/powershell/module/servicefabric/remove-servicefabricreplica)
    * [FabricClient.FaultManagementClient.RemoveReplicaAsync](https://learn.microsoft.com/en-us/dotnet/api/system.fabric.fabricclient.faultmanagementclient.removereplicaasync)
    * [Remove Replica – Microsoft Learn](https://learn.microsoft.com/en-us/rest/api/servicefabric/sfclient-api-removereplica)
  * Support for stateless instance removal will be fully removed approximately one year after the release of 11.3:
  * **Customer impact:** Using these APIs after upgrading to version 11.3 will trigger deprecation warnings. Full removal of support is targeted for a future release, approximately one year post-11.3.
    * Scripts and tools depending on the deprecated APIs may fail in future versions.
    * Customers should begin migrating to the new APIs as soon as they become available to prevent disruptions in automation workflows.
    * Alternate APIs to support stateless instance removal will be announced in a 11.3 release.
  * **Migration path:** Service Fabric 11.3 introduces new APIs and a compatibility switch to ensure existing scripts continue functioning after upgrading, while allowing time to transition.
    * Full details about replacements and migration guidance will be included in the 11.3 release notes and documentation.

* Starting with Service Fabric version 12, the managed assemblies of the public SDK no longer have the assembly CLSCompliantAttribute. Service Fabric APIs commonly use unsigned integer types that aren't CLS-compliant. The Service Fabric SDK doesn't support any .NET languages other than C#, so the CLS compliance isn't applicable. Users building CLS-compliant assemblies that expose Service Fabric types in their public API will get the CS3003 build warning.
  * User assemblies exposing Service Fabric types in their public APIs can do one of the following:
    * Suppress CS3003 build warnings in project files by adding `<NoWarn>3003</NoWarn>`
    * Remove `[assembly: CLSCompliant(true)]`
    * Mark public APIs exposing Service Fabric types as `[CLSCompliant(false)]`

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

Runtime:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.1.208.1.exe

SDK:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.1.208.msi

Cab:
https://download.microsoft.com/download/b/0/b/b0bccac5-65aa-4be3-ab13-d5ff5890f4b5/11.1.208.1/MicrosoftServiceFabric.11.1.208.1.cab

Package:
https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.1.208.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.1.208.1.zip
 
Goalstate:
https://download.microsoft.com/download/7/d/1/7d1d1511-59a4-4933-8187-40c20065aa29/11.1.208.1/goalstate.11.1.208.1.json
