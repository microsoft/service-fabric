# Microsoft Azure Service Fabric 7.0 Fourth Refresh Release Notes

This release includes the bug fixes and features described in this document. This release includes runtime, SDKs and Windows Server Standalone deployments to run on-premises. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows | 7.0.469.1 <br> 7.0.470.9590 |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | 7.0.470.9590 |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration|4.0.470 <br>7.0.470 <br> 4.0.470 <br> 4.0.470 |
|Java SDK  |Java for Linux SDK  | 1.0.5 |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15 <br> 9.0.0 |

## Contents 

Microsoft Azure Service Fabric 7.0 Fourth Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Repositories and Download Links](#repositories-and-download-links)

## Key Announcements

## Breaking Changes

Service Fabric versions below the Service Fabric 7.0 Third Refresh Release (Windows:7.0.466.9590, Linux:7.0.465.1) must first update to the 7.0 Third Refresh before upgrading to newer versions if the cluster has settings as depicted below.

```XML
<!-- When no thumbprint is set up for primary account, the cluster must upgrade to 7.0 Third Refresh Release first. -->
<Section Name="Security">
  <Parameter Name="ServerAuthCredentialType" Value="X509" /> <!-- Value other than None -->
</Section>
<Section Name="FileStoreService">
  <Parameter Name="PrimaryAccountNTLMX509Thumbprint" Value="" /> <!-- Empty or not set. -->
  <Parameter Name="CommonName1Ntlmx509CommonName" Value="" /> <!-- Empty or not set. -->
</Section>
```

## Service Fabric Common Bugs

| Versions | IssueType | Description | Resolution | 
|----------|-----------|-|-|
| **Windows 7.0.470.9590** | **Bug** | Security setting CrlCheckingFlags not used by Federation | **Impact**: If the user specified a setting for CrlCheckingFlags, it would not be used by Federation. <br>**Fix**: Deprecate the internal override of CrlCheckingFlags <br> **Workaround**: Set internal config setting Federation/X509CertChainFlags to the same value as CrlCheckingFlags |
| **Windows Server 2019** | **Bug** | Application deployments sometimes failed on Server 2019 clusters  when AnonymousAccessEnabled set to true |  **Impact**: Application deployed on Server 2019 clusters failed when AnonymousAccessEnabled set to true. <br> **Fix**:  In general, AnonymousAccessEnabled should be set to false by default unless there is an identified need for it. On Windows Server 2019 anonymous access is disabled by default by the OS, so if it is needed the corresponding registry key should be set.
| **Windows 7.0.470.9590** | **Bug** | Cluster Creation: The network name cannot be found | **Impact**: Configuring a cluster using 'localhost' fails on some machines due to "network name cannot be found" when resolving UNC paths. <br> **Fix**: Removing the cases where we try to create a UNC path using localhost machine name. As part of this change an optimization is added to reduce DNS lookups for hostnames, which eliminates the overhead in determining whether the target machine being configured is local. |
| **Windows 7.0.470.9590** | **Bug** | Standalone Clusters: Configuration upgrades were in some cases blocked for secure clusters | **Impact**: Standalone cluster customers with secure clusters were blocked from running configuration upgrade. https://github.com/Azure/service-fabric-issues/issues/1298 <br> **Fix**: Flattened Security CertificateInformation & WindowsIdentities section handling & validation interpretation such that null & empty sections are handled equivalently. Removed the generation of empty security sections in FabricUOS state when they are not being used, preventing them from being returned unexpectedly when users query latest configuration. <br> **Workaround**: Add blank security sections to the upgraded configuration to match hidden internal state. |
| **Windows 7.0.470.9590** | **Bug** | Standalone Clusters: Timeout parameter handling for Cluster Creation. | **Fix**: Consolidate CreateCluster timeout parameters TimeoutInSeconds & TimeoutSec, add tracing via verbose logging to identify old parameter as deprecated. |
| **Windows 7.0.470.9590** | **Feature** | Standalone Clusters: AddNode has new override parameters BypassUpgradeStateValidation, TimeoutInSeconds, Force, NoCleanupOnFailure. | **Brief desc**: BypassUpgradeStateValidation: Workaround flag for when AddNode is blocked by CM data loss state, which results in interpreting that the cluster baseline upgrade has not completed. <br> TimeoutInSeconds: In parity with timeout behavior of CreateCluster. <br> Force: In parity with Force parameter of CreateCluster (overwrites preexisting data root). <br> NoCleanupOnFailure: In parity with NoCleanupOnFailure parameter of CreateCluster (no rollback/installation cleanup in case of failure). |
| **Windows 7.0.470.9590** | **Bug** | Standalone Clusters: AddNode now validates the node has joined. | **Impact**: AddNode lacked a validation after completing join of the new node to the cluster. <br> **Fix**: AddNode.ps1 now verifies the node has joined. |
| **Windows 7.0.470.9590** | **Bug** | Standalone Clusters: AddNode for secure clusters was failing with incorrect error message. | **Impact**: When AddNode hit connectivity issues with the cluster it emitted an erroneous error message related to erroneous message "Runtime package cannot be downloaded. Check your internet connectivity.". <br> **Fix**: The correct error message is now being written. |
| **Windows 7.0.470.9590** | **Bug** | Standalone Clusters: Azure storage connection string Diagnostics configuration was not working. | **Impact**: Cluster creation would fail when the configuration diagnostics section contained an Azure storage connection string. <br> **Fix**: Standalone DeploymentComponents package was missing System.Fabric.Dca.dll; this has been added to the package. <br> **Workaround**: Manually drop System.Fabric.Dca.dll inside DeploymentComponents directory in standalone cluster deployment package. |
| **Windows 7.0.470.9590** | **Bug** | Standalone Clusters: Trace gap in standalone cluster deployment traces. | **Impact**: CreateCluster/AddNode/RemoveCluster deployment traces had trace gaps due to conflicting trace sessions in submodule code. <br> **Fix**: SF Standalone operations now write contiguous traces that span the entire operation. Fixed known issues related to trace gaps & fragmentation when switching between trace types. |
| **Windows 7.0.470.9590** | **Bug** | SF runtime hits conflicts upgrading/reinstalling since ese.dll is held by external applications. | **Impact**: Ese.dll is being held by various agents including event monitoring applications in production (i.e. MonAgent, event viewer), which results in ownership contention and an increased rate of reboots that aimed to autoresolve these conflicts. This has also been a blocker for customers upgrading runtime as part of dev box SDK upgrade. <br> **Fix**: We have introduced a workaround to remove these global environment references to avoid contention against this dll, and have introduced automitigations where this has been problematic in the past. <br> **Workaround**: Reboot machine or stop all contending sources trying to load SF's carried copy of ese.dll. |
| **Windows .NET SDK** | **Bug** | Runtime upgrade was blocked by old SDK residing on the machine. | **Impact**: Upgrading Service Fabric runtime on a machine with old SDK installed had previously been blocked by a version compatibility check. <br> **Fix**: Now a modal dialog is presented to alert the user that the compatibility check has failed but does not block the installation. <br> **Workaround**: Uninstall SDK before attempting installation of SF runtime. |
| **Windows .NET SDK** | **Bug** | SF Runtime SDK setup was blocked when running as SYSTEM. | **Impact**: SDK setup in some automated environments runs as SYSTEM, which cannot set Powershell execution policy and this was required to run setup. <br> **Fix**: A parameter has been added to the runtime installation exe: /SkipPSExecutionPolicy, which bypasses setting Powershell execution policy. |
| **Windows .NET SDK** | **Bug** | Runtime SDK installation wedges when runtime files have only been partially wiped. | **Impact**: In some cases the SF SDK becomes partially uninstalled which leaves behind some key components that cause the subsequent reinstallation to self-identify as already installed which blocks reinstallation. <br> **Fix**: SDK SF runtime installer now autoresolves corrupted runtime installations where registry entries remain but files have been removed, in order to unblock installation of the new version. <br> **Workaround**: Manually delete the Service Fabric 'Add-Remove Programs' registry uninstall entry, as well as the HKLM Software entry for Service Fabric. |
| **Windows 7.0.470.9590** | **Bug** | Fabric Upgrade Service keeps retrying application upgrade after rollback. | **Impact**: When an operation is currently running and a new one gets queued for the same resource, the new operation never gets added to the operations map. If the second operation rolls back it will keep retrying until it times out instead of failing. <br> **Fix**: Update operation map with new upgrade. <br> **Workaround**: Manually restart Fabric Upgrade Service primary replica. |
| **Windows 7.0.470.9590** | **Bug** | Support logs and files are not uploaded to Azure Storage from FIPS enabled machines. | **Impact**: No support logs are uploaded by FabricDCA process from FIPS enabled machines. <br> **Fix**: FabricDCA now configures the Azure Storage API it uses for uploading to Azure Storage to not use the default MD5 hashing algorithm which is not compliant with FIPS. <br> **Workaround**: Disable FIPS. |


## Repositories and Download Links

The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

|Area |Package | Version | Repository |Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up |7.0.469.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 
|| Windows Developer Set-up| 7.0.470.9590 | N/A |https://download.microsoft.com/download/0/0/f/00fbca28-0a64-4c9a-a3a3-b11763ee17e5/MicrosoftServiceFabric.7.0.470.9590.exe |
|Service Fabric for Windows Server |Service Fabric Standalone Installer Package |7.0.470.9590 |N/A | https://download.microsoft.com/download/8/3/6/836E3E99-A300-4714-8278-96BC3E8B5528/7.0.470.9590/Microsoft.Azure.ServiceFabric.WindowsServer.7.0.470.9590.zip |
||Service Fabric Standalone Runtime |7.0.470.9590 |N/A |https://download.microsoft.com/download/B/0/B/B0BCCAC5-65AA-4BE3-AB13-D5FF5890F4B5/7.0.470.9590/MicrosoftAzureServiceFabric.7.0.470.9590.cab |
|.NET SDK |Windows .NET SDK | 4.0.470 |N/A |https://download.microsoft.com/download/0/0/f/00fbca28-0a64-4c9a-a3a3-b11763ee17e5/MicrosoftServiceFabricSDK.4.0.470.msi |
||Microsoft.ServiceFabric | 7.0.470 |N/A |https://www.nuget.org |
||Reliable Services and Reliable Actors<br>\-Microsoft.ServiceFabric.Services<br>\-Microsoft.ServiceFabric.Services.Remoting<br>\-Microsoft.ServiceFabric.Services.Wcf <br>\-Microsoft.ServiceFabric.Actors <br>\-Microsoft.ServiceFabric.Actors.Wcf |4.0.470|https://github.com/Azure/service-fabric-services-and-actors-dotnet |https://www.nuget.org |
||ASP.NET Core Service Fabric integration<br>\-Microsoft.ServiceFabric.Services.AspNetCore.*|4.0.470 |https://github.com/Azure/service-fabric-aspnetcore |https://www.nuget.org |
||Data, Diagnostics and Fabric transport<br>\-Microsoft.ServiceFabric.Data <br>\-Microsoft.ServiceFabric.Data.Interfaces <br>\-Microsoft.ServiceFabric.Diagnostics.Internal <br>\-Microsoft.ServiceFabric.FabricTransport/Internal | 4.0.470 |N/A| https://www.nuget.org |
||Microsoft.ServiceFabric.Data.Extensions | 4.0.470 |N/A |https://www.nuget.org |
|Java SDK |Java SDK | 1.0.5 |N/A |https://mvnrepository.com/artifact/com.microsoft.servicefabric/sf-actors/1.0.5 |
|Eclipse |Service Fabric plug-in for Eclipse | 2.0.7 | N/A |N/A |
|Yeoman |Azure Service Fabric Java generator | 1.0.7 |https://github.com/Azure/generator-azuresfjava |N/A |
||Azure Service Fabric C# generator | 1.0.9 |https://github.com/Azure/generator-azuresfcsharp |N/A |
||Azure Service Fabric guest executables generator |1.0.1 |https://github.com/Azure/generator-azuresfguest |N/A|
||Azure Service Fabric Container generators | 1.0.1 |https://github.com/Azure/generator-azuresfcontainer |N/A |
|CLI |Service Fabric CLI | 9.0.0 |https://github.com/Azure/service-fabric-cli |https://pypi.python.org/pypi/sfctl |
|PowerShell |AzureRM.ServiceFabric | 0.3.15 |https://github.com/Azure/azure-powershell/tree/preview/src/ResourceManager/ServiceFabric |https://www.powershellgallery.com/packages/AzureRM.ServiceFabric/0.3.15  |
