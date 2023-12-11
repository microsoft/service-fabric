# Microsoft Azure Service Fabric 10.1 Release Notes

This release will only be available through manual upgrades. Clusters set to automatic upgrades will not receive this release. For how to configure upgrades, please see [classic](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-upgrade) or [managed](https://docs.microsoft.com/en-us/azure/service-fabric/how-to-managed-cluster-configuration) documentation.

## Contents
* [Service Fabric Packages and Versions](#service-fabric-packages-and-versions)
* [Service Fabric Feature and Bug Fixes](#service-fabric-feature-and-bug-fixes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)

## Service Fabric Packages and Versions
The following packages and versions are part of this release:

| Service | Platform | Version |
|-|-|-|
| Service Fabric Runtime | Ubuntu 20 <br> Windows | 10.1.1507.1 <br> 10.1.1541.9590 |
| Service Fabric for Windows Server | Service Fabric Standalone Installer Package | 10.1.1541.9590 |
| .NET SDK | Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration | 7.1.1541 <br> 10.1.1541 <br> 7.1.1541 <br> 7.1.1541 |
| Java SDK | Java for Linux SDK | 1.0.6 |
| Service Fabric PowerShell and CLI | AzureRM PowerShell Module <br> SFCTL | 0.3.15 <br> 11.0.1 |

## Service Fabric Feature and Bug Fixes
| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| Windows -<br>10.1.1541.9590 <br> Ubuntu 20 -<br>10.1.1507.1 | Feature | Access/Authorization | Brief Description: Service Fabric runtime defines two client roles - Admin and Client. The Admin role is highly privileged, and undistinguishable from the runtime itself which can be problematic in shared clusters, where all tenants have Admin privileges and can perform unintended destructive operations on the services of another tenant. In this release we are introducing third client role - ElevatedAdmin, which, combined with properly configured Security/ClientAccess section of cluster manifest, can prevent the described scenario. |
| Windows -<br>10.1.1541.9590 <br> Ubuntu 20 -<br>10.1.1507.1 | Feature | Disable Service Fabric Node | Brief Description: Disable-ServiceFabricNode now accepts an optional 'description' parameter where one can provide more information about node deactivation. Whether 'description' is provided or not, it always captures user-agent information. <br>Solution: Updated Fabric Client, handlers, Fabric Client wrapper and COM data |
| Windows -<br>10.1.1541.9590 <br> Ubuntu 20 -<br>10.1.1507.1 | Feature | SFX/SFE | Brief Description: Service Fabric now emits a health event visible in SFX/SFE when Sessions are exhausted. |
| Windows -<br>10.1.1541.9590 <br> Ubuntu 20 -<br>10.1.1507.1 | Feature | CRM | Cluster Resource Manager (CRM) is introducing sensitivity feature, which allows a customer to set a stateful service replica as maximum sensitive. This feature is designed to provide maximum protection for the sensitive replica against movement or swap. The movement or swap of a max sensitive replica is a rare occurrence and only happens in the following specific cases: <br> - FD/UD constraint violation only if FD/UD is set to hard constraint <br> - Replica swap during upgrade <br> - Node capacity violation with only MSR(s) on the node (i.e., if any other non-MSR is present on the node, the MSR is not movable.) |
| Windows -<br>10.1.1541.9590 <br> Ubuntu 20 -<br>10.1.1507.1 | Feature | Throttling | Brief Description: This feature allows the weight of InBuild Auxiliary replicas to be set when applied to InBuild throttling. A higher weight means that an InBuild Auxiliary replica will take up more of the InBuild limit, and likewise a lower weight would consume less of the limit, allowing more replicas to be placed InBuild before the limit is reached. |

## Retirement and Deprecation Path Callouts

* Ubuntu 18.04 LTS has reached its 5-year end-of-life window on June-2023. Service Fabric runtime has dropped support for 18.04 LTS after the published date, and we recommend moving your clusters and applications to supported versions listed here: [Service Fabric supported Linux versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions#supported-linux-versions-and-support-end-date)

* Previously communicated, Service Fabric runtime had planned to remove Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older, from the package Download Center in July 2023. We would like to inform you that this timeline has been extended, and the removal will now take place in January 2024.

* Service Fabric runtime will soon be archiving and removing Service Fabric runtime versions less than 7.2 and older, as well as the corresponding SDK version 4.2 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors. 
  * To prevent disruption of workloads, create a new cluster using the following steps:
    * [Create a Service Fabric cluster using ARM template](https://learn.microsoft.com/en-us/azure/service-fabric/quickstart-cluster-template)
    * [Create a Standalone cluster](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-creation-for-windows-server)
    * Install the supported version of Service Fabric SDK based on the Runtime version installed on the cluster.
