# Microsoft Azure Service Fabric 10.0 Cumulative Update 4.0 Release Notes

This release will only be available through manual upgrades. Clusters set to automatic upgrades will not receive this release. For how to configure upgrades, please see [classic](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-upgrade) or [managed](https://docs.microsoft.com/en-us/azure/service-fabric/how-to-managed-cluster-configuration) documentation.

## Contents
* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)

## Service Fabric Packages and Versions
The following packages and versions are part of this release:

| Service | Platform | Version |
|-|-|-|
| [Service Fabric Runtime](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.10.0.2382.9590.exe) | Ubuntu 20 <br> Windows | 10.0.2261.1 <br> 10.0.2382.9590 |
| [Service Fabric for Windows Server](https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/10.0.2382.9590/Microsoft.Azure.ServiceFabric.WindowsServer.10.0.2382.9590.zip) | Service Fabric Standalone Installer Package | 10.0.2382.9590 |
| [.NET SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.0.2382.msi) | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 7.0.2382 <br> 10.0.2382 <br> 7.0.2382 <br> 7.0.2382 |
| [Java SDK](https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.0.2382.msi) | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module <br> SFCTL | 0.3.15 <br> 11.0.1 |

## Service Fabric Feature and Bug Fixes
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| Windows -<br>10.0.2382.9590 <br> Ubuntu 20 -<br>10.0.2261.1 | Feature | Health | **Brief Description:** Remap hardware related JET error form StoreLvidLimitHit to a new "FABRIC_E_STORE_OUT_OF_LONG_VALUE_IDS" exception and reflect the change in the health event. <br> **Feature/Bug Impact:** Improve visibility of LVID exhaustion with a user-friendly exception and internally consistent naming. <br> **Solution/Fix:** Long Value IDs are used by ESE (internal storage engine used by KVS) to keep track of values greater than 5120 bytes. These IDs can be exhausted over a period and the database needs to be compacted to recover them. This exhaustion of IDs is currently an internal error code which cannot be expressed as a public exception or a status code. This work items replaces the internal error code with a new public error code FABRIC_E_STORE_OUT_OF_LONG_VALUE_IDS. |
| Windows -<br>10.0.2382.9590 <br> Ubuntu 20 -<br>10.0.2261.1 | Feature | MaxJetInstances | **Brief Description:** MaxJetInstances is marked as deprecated and removed from usage. <br> **Feature/Bug Impact:** As an unused setting with no reason to set outside of the default state, we removed this setting to prevent possible future issues. <br> **Solution/Fix:** This fixes any potential issues from a customer setting this variable to an unusable setting. |
| Windows -<br>10.0.2382.9590 <br> Ubuntu 20 -<br>10.0.2261.1 | Feature | Key Value Store (KVS) | **Brief Description:** The initial defragmentation of KVS replicas will now be distributed between an immediate defragmentation and the setting of MaxDefragFrequencyInMinutes which is by default 30 minutes. All subsequent defragmentations will use the MaxDefragFrequencyInMinutes value. Previously all initial  defragmentations were immediately scheduled upon open. <br> **Feature/Bug Impact:** A random integer is now generated in a range of (0, MaxDefragFrequencyInMinutes) to determine the initial delay for defragmentation instead of happening immediately to prevent scenarios where multiple replicas are open at the same time leading to increased disk usage. <br> **Solution/Fix:** This prevents scenarios where multiple replicas are open at the same time leading to increased disk usage even though customers have set a setting for a MaxDefragFrequencyInMinutes to once a day or even once a week. |
| Windows -<br>10.0.2382.9590 <br> Ubuntu 20 -<br>10.0.2261.1 | Feature | Health | **Brief Description:** Jet_errTooManyInstances now has health reporting, and associated tests. <br> **Feature/Bug Impact:** To add to the usability of the recently added TooManyInstances error code, added health reporting and tests to ensure proper health reporting. <br> **Solution/Fix:** Error codes need health reporting to keep current status of the SF environment as well as perform appropriate repair actions, this enables that reporting to provide visibility when the error is occuring. |
| Windows -<br>10.0.2382.9590 <br> Ubuntu 20 -<br>10.0.2261.1 | Bug | Cluster Upgrades | **Brief Description:** Validates correct platform-specific settings during cluster upgrade. <br> **Feature/Bug Impact:** Before this change, some Linux-specific settings were assumed to be valid for Windows clusters and vice-versa. <br> **Solution/Fix:** Settings correct for the platform are now used during cluster upgrade validation. Customers who have settings invalid for their platform in their cluster manifest, will have to fix the configuration first before upgrading to the new SF runtime version. |

## Retirement and Deprecation Path Callouts

* Service Fabric runtime will discontinue support for the Java SDK soon. For a smooth transition, we strongly recommend users to shift to Azure Service Fabric .NET SDK. If your current setup is based on the Service Fabric Java SDK, we suggest starting migration plans to smoothly switch to the Azure Service Fabric .NET SDK. Although applications using the Java SDK will continue to work, we highly recommend adopting the SF .NET SDK for optimal outcomes.

* Ubuntu 18.04 LTS has reached its 5-year end-of-life window on June-2023. Service Fabric runtime has dropped support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#supported-linux-versions-and-support-end-date)

* Previously communicated, Service Fabric runtime had planned to remove Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older, from the package Download Center in July 2023. We would like to inform you that this timeline has been extended, and the removal will now take place in January 2024.

* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors. 
  * To prevent disruption of workloads, create a new cluster using the following steps:
    * [Create a Service Fabric cluster using ARM template](https://learn.microsoft.com/en-us/azure/service-fabric/quickstart-cluster-template)
    * [Create a Standalone cluster](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)
    * Install the supported version of Service Fabric SDK based on the Runtime version installed on the cluster.

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Getting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

Run Time:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabric.10.0.2382.9590.exe

SDK:
https://download.microsoft.com/download/b/8/a/b8a2fb98-0ec1-41e5-be98-9d8b5abf7856/MicrosoftServiceFabricSDK.7.0.2382.msi

Cab:
https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/10.0.2382.9590/MicrosoftAzureServiceFabric.10.0.2382.9590.cab

Package:
https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/10.0.2382.9590/Microsoft.Azure.ServiceFabric.WindowsServer.10.0.2382.9590.zip

Goalstate:
https://download.microsoft.com/download/7/D/1/7D1D1511-59A4-4933-8187-40C20065AA29/10.0.2382.9590/goalstate.10.0.2382.9590.json