# Microsoft Azure Service Fabric 7.2 Third Refresh Release Notes

This release includes stability fixes as described in this document. This release is only available through **manual upgrades** on Service Fabric Windows clusters.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | N/A <br> N/A <br> 7.2.433.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.2.433.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.2.434 <br> 7.2.434 <br> 4.2.434 <br> 4.2.434 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 10.0.0 |

## Content 

Microsoft Azure Service Fabric 7.2 Third Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Upcoming Breaking Changes](#breaking-changes)
* [Service Fabric Runtime](#service-fabric-runtime)

## Key Announcements

* **Fixed stability issues in FabricDNS** - This release fixes stability issues introduced to FabricDNS in the 7.2 release.  Issues seen in this regression include an increased rate of slow/failed DNS requests, and FabricDNS crashing more than usual.
* This release includes an additional fix for a memory leak in very specific scenarios when using remoting V2 and creating and destroying services dynamically.

## Upcoming Breaking Changes

- Service Fabric 7.2 and higher runtime drops support for .NET core Service Fabric apps running with .NET core 2.2 runtime. .NET core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET core LTS version 3.1.
- .NET core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET core apps.  This has no impact on Service Fabric .NET Framework SDK.
- .NET 5 apps are currently not supported, support for .NET 5 applications will be added in the Service Fabric 8.0 release.
- We will be deprecating the latest tag for [**OneBox container images**](https://hub.docker.com/_/microsoft-service-fabric-onebox).The new scheme for containers will substitute the current naming scheme ‘latest’ to be replaced with an OS-targeted latest set to explicitly identify the base OS for latest SF releases:
    - mcr.microsoft.com/service-fabric/onebox:u16
    - mcr.microsoft.com/service-fabric/onebox:u18
- The default action of deleting uploaded application package is changed. If the application package is registered successfully, the uploaded application package is deleted by default. Before this default action changes, the uploaded application package was deleted either by calling to [Remove-ServiceFabricApplicationPackage](https://docs.microsoft.com/en-us/powershell/module/servicefabric/remove-servicefabricapplicationpackage) or by using ApplicationPackageCleanupPolicy parameter explicitly with [Register-ServiceFabricApplicationType](https://docs.microsoft.com/en-us/powershell/module/servicefabric/register-servicefabricapplicationtype)
- Service Fabric 7.2 will become the baseline release for future development of image validation. When the feature is activated in future version e.g., Service Fabric 7.3, the cluster is not desired to rolled back down to 7.1 or lower. The version 7.2 is the baseline to which the cluster can downgrade.

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.2.433.9590** | **Bug** |FabricDNS regression introduced in 7.2.|**Brief desc**: FabricDNS would not behave as expected after upgrading to 7.2.<br> **Impact**: Issues seen in this regression include an increased rate of slow/failed DNS requests, and FabricDNS crashing more than usual. <br> **Workaround**: Upgrade to 7.2 CU2/CU3. <br> **Fix**: This regression has been fixed as part the CU2 release.