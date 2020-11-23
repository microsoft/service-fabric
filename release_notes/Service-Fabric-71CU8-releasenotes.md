# Microsoft Azure Service Fabric 7.1 Eighth Refresh Release Notes

This release includes bug fixes as described in this document. **This release is made available only and will not be available through automatic upgrades**.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 16 <br> Ubuntu 18 <br> Windows | N/A <br> N/A <br> 7.1.503.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | N/A |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| 4.1.458 <br> 7.1.458 <br> 4.1.458 <br> 4.1.458 |
|Java SDK  |Java for Linux SDK  | 1.0.6 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL | 0.3.15 <br> 10.0.0 |

## Content 

Microsoft Azure Service Fabric 7.1 Eighth Refresh Release Notes

## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows 7.1.503.9590** <br> | **Bug** | Assert when error code not read/checked | **Brief desc**: This release includes a fix for an assert that happens because an error code was not read/checked.  <br> **Impact**: Assert occurs when the error code cannot be read/checked. <br> **Workaround**: N/A <br> **Fix**: Upgrade to the latest version.
