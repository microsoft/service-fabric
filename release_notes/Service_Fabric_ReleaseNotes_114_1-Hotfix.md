# Microsoft Azure Service Fabric 11 CU4_1 Hotfix Release Notes

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
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.4.268.1.exe) | Ubuntu 22 <br> Windows | 11.4.268.4 <br> 11.4.268.1 |
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.4.268.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.4.268.1.zip) | Service Fabric Standalone Installer Package | 11.4.268.1 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.4.268.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 8.0.0 <br> 11.4.268.1 <br> 8.0.0 <br> 8.0.0 |
| [Java SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.4.268.msi) | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module<br>SFCTL | 0.3.15 <br> 11.0.1 |

## Service Fabric Feature and Bug Fixes

The Cumulative Update 4.0_1 Hotfix for Azure Service Fabric 11 included various security fixes.

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
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.11.4.268.1.exe

SDK:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.8.4.268.msi

Cab:
https://download.microsoft.com/download/b/0/b/b0bccac5-65aa-4be3-ab13-d5ff5890f4b5/11.4.268.1/MicrosoftServiceFabric.11.4.268.1.cab

Package:
https://download.microsoft.com/download/8/3/6/836e3e99-a300-4714-8278-96bc3e8b5528/11.4.268.1/Microsoft.Azure.ServiceFabric.WindowsServer.11.4.268.1.zip
 
Goalstate:
https://download.microsoft.com/download/7/d/1/7d1d1511-59a4-4933-8187-40c20065aa29/11.4.268.1/goalstate.11.4.268.1.json
