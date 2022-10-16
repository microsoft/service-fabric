# Microsoft Azure Service Fabric Breaking Changes

* [Breaking Changes 82CU51](#breaking-changes-90cu21)
* [Breaking Changes 82CU51](#breaking-changes-82cu51)
* [Breaking Changes 82CU1](#breaking-changes-82cu1)
* [Breaking Changes 82](#breaking-changes-82)
* [Breaking Changes 72CU6](#breaking-changes-72cu6)
* [Breaking Changes 72CU2](#breaking-changes-72cu2)
* [Breaking Changes 72](#breaking-changes-72)
* [Breaking Changes 71CU3](#breaking-changes-71cu3)
* [Breaking Changes 71](#breaking-changes-71)
* [Breaking Changes 70CU4](#breaking-changes-70cu4)
* [Breaking Changes 70](#breaking-changes-70)
* [Breaking Changes 65CU2](#breaking-changes-65cu2)
* [Breaking Changes 65](#breaking-changes-65)

## Breaking Changes 90CU21

| Versions | Area | Description | 
|-|-|-|
| **Ubuntu 18 - 9.0.1086.1<br>Ubuntu 20 - 9.0.1086.1** | Service Fabric Runtime dependency | Customers running Azure Service Fabric Linux Clusters will not be able to scale out or reimage (manual or auto via VMSS OS Image Update) nodes or create new Linux clusters without upgrading to this version of Service Fabric (8.2.1483.1). Also, if your application requires JDK, then you will need to install a JDK of your choice since SF will not install the JDK as part of the SF installation.<br>
Starting 8/19/2022, the following [versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions) will be deprecated and will not be available on Linux for new cluster creation or node reimage/scale out operations:<br> <ul><li>9.0 CU2/CU1/RTO</li><li>8.2 CU4/CU3/CU2.1/CU2/CU1/RTO</li></ul><br>Details: Service Fabric Runtime (for Linux) placed a dependency on Zulu for Azure build of OpenJDK which is no longer supported as announced in their dev [blog](https://devblogs.microsoft.com/java/end-of-updates-support-and-availability-of-zulu-for-azure/). Since the creation of new Linux clusters or scale-out/reimage of nodes required all the dependencies to be installed, these workflows broke when the dependency became unavailable.<br>Beginning with this release, Service Fabric Runtime will no longer install any JDK dependency which will ensure that customer can scale out, reimage, or create new Linux clusters. If your application needs JDK, please utilize an alternate mechanism to provision a JDK. To get the latest Service Fabric Runtime releases with this change, follow the [upgrade documentation](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-tutorial-upgrade-cluster). If you depend on a JDK, please be sure to install one of your choices. For more information see: [Breaking change for Azure Service Fabric Linux customers](https://techcommunity.microsoft.com/t5/azure-service-fabric-blog/breaking-change-for-azure-service-fabric-linux-customers/ba-p/3604678)


## Breaking Changes 82CU51

| Versions | Area | Description | 
|-|-|-|
| **Ubuntu 18 - 8.2.1483.1** | Service Fabric Runtime dependency | Customers running Azure Service Fabric Linux Clusters will not be able to scale out or reimage (manual or auto via VMSS OS Image Update) nodes or create new Linux clusters without upgrading to this version of Service Fabric (8.2.1483.1). Also, if your application requires JDK, then you will need to install a JDK of your choice since SF will not install the JDK as part of the SF installation.<br>
Starting 8/19/2022, the following [versions](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-versions) will be deprecated and will not be available on Linux for new cluster creation or node reimage/scale out operations:<br> <ul><li>9.0 CU2/CU1/RTO</li><li>8.2 CU4/CU3/CU2.1/CU2/CU1/RTO</li></ul><br>Details: Service Fabric Runtime (for Linux) placed a dependency on Zulu for Azure build of OpenJDK which is no longer supported as announced in their dev [blog](https://devblogs.microsoft.com/java/end-of-updates-support-and-availability-of-zulu-for-azure/). Since the creation of new Linux clusters or scale-out/reimage of nodes required all the dependencies to be installed, these workflows broke when the dependency became unavailable.<br>Beginning with this release, Service Fabric Runtime will no longer install any JDK dependency which will ensure that customer can scale out, reimage, or create new Linux clusters. If your application needs JDK, please utilize an alternate mechanism to provision a JDK. To get the latest Service Fabric Runtime releases with this change, follow the [upgrade documentation](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-tutorial-upgrade-cluster). If you depend on a JDK, please be sure to install one of your choices. For more information see: [Breaking change for Azure Service Fabric Linux customers](https://techcommunity.microsoft.com/t5/azure-service-fabric-blog/breaking-change-for-azure-service-fabric-linux-customers/ba-p/3604678)

## Breaking Changes 82CU1

| Versions | Area | Description | 
|-|-|-|
| **Windows - 8.2.1363.9590<br>Ubuntu 18 - 8.2.1204.1** | Windows Server EOL | Support for Windows Server 2016 and Windows Server 1809 will be discontinued in future Service Fabric releases. We recommend updating your cluster VMs to Windows Server 2019.

## Breaking Changes 82

| Versions | Area | Description | 
|-|-|-|
| **Windows - 8.2.1235.9590<br>Ubuntu 18 - 8.2.1124.1** | ASP.NET Core 1.0 Support | ASP.NET Core 1.0 Support: Service Fabric ASP.NET Core packages are built against ASP.NET Core 1.0 binaries which are out of support. Starting Service Fabric 8.2, we will be building Service Fabric ASP.NET Core packages against .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, .NET Framework 4.6.1. As a result, they can be used only to build services targeting .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, >=.NET Framework 4.6.1 respectively.<br> 
For .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, Service Fabric ASP.NET Core will be taking dependency on Microsoft.AspNetCore.App shared framework, whereas for NetFx target frameworks >=.NET Framework 4.6.1, Service Fabric ASP.NET Core will be taking dependency on ASP.NET Core 2.1 packages. <br>
The package Microsoft.ServiceFabric.AspNetCore.WebListener will no longer be shipped, Microsoft.ServiceFabric.AspNetCore.HttpSys package should be used instead 


## Breaking Changes 72CU6

| Versions | Area | Description | 
|-|-|-|
| **Windows - 7.2.457.9590<br>Ubuntu 16 - 7.2.456.1<br>Ubuntu 18 - 7.2.456.1804** | Windows Server EOL | Support for Windows Server 2016 and Windows Server 1809 will be discontinued in future Service Fabric releases. We recommend updating your cluster VMs to Windows Server 2019.


## Breaking Changes 72CU2

| Versions | Area | Description | 
|-|-|-|
| **Windows - 7.2.432.9590<br>Ubuntu 16 - 7.2.431.1804<br>Ubuntu 18 - 7.2.431.1804** | .Net Runtime EOL | Service Fabric 7.2 and higher runtime drops support for .NET core Service Fabric apps running with .NET core 2.2 runtime. .NET core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET core LTS version 3.1.
| **Windows - 7.2.432.9590<br>Ubuntu 16 - 7.2.431.1804<br>Ubuntu 18 - 7.2.431.1804** | .NET core runtime LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric releases after that date will drop support for Service Fabric apps running with .NET core 2.1 runtime. Service Fabric .NET SDK will take a dependency on .Net runtime 3.* features to support Service Fabric .NET core apps.  This has no impact on Service Fabric .NET Framework SDK. 


## Breaking Changes 72

| Versions | Area | Description | 
|-|-|-|
| **Windows - 7.2.413.9590<br>Ubuntu 16 - NA<br>Ubuntu 18 - NA** | .Net Runtime EOL | .NET 5 apps are currently not supported, support for .NET 5 applications will be added in the Service Fabric 8.0 release. 
| **Windows - 7.2.413.9590<br>Ubuntu 16 - NA<br>Ubuntu 18 - NA** | SF OneBox Container images | We will be deprecating the latest tag for [OneBox container images](https://hub.docker.com/_/microsoft-service-fabric-onebox). The new scheme for containers will substitute the current naming scheme ‘latest’ to be replaced with an OS-targeted latest set to explicitly identify the base OS for latest SF releases: <br> mcr.microsoft.com/service-fabric/onebox:u16 <br> mcr.microsoft.com/service-fabric/onebox:u18 
| **Windows - 7.2.413.9590<br>Ubuntu 16 - NA<br>Ubuntu 18 - NA** | Application package | The default action of deleting uploaded application package is changed. If the application package is registered successfully, the uploaded application package is deleted by default. Before this default action changes, the uploaded application package was deleted either by calling to [Remove-ServiceFabricApplicationPackage](https://learn.microsoft.com/en-us/powershell/module/servicefabric/remove-servicefabricapplicationpackage?view=azureservicefabricps) or by using ApplicationPackageCleanupPolicy parameter explicitly with [Register-ServiceFabricApplicationType](https://learn.microsoft.com/en-us/powershell/module/servicefabric/register-servicefabricapplicationtype?view=azureservicefabricps)
| **Windows - 7.2.413.9590<br>Ubuntu 16 - NA<br>Ubuntu 18 - NA** | SF Baseline version | Service Fabric 7.2 will become the baseline release for future development of image validation. When the feature is activated in future version e.g., Service Fabric 7.3, the cluster is not desired to rolled back down to 7.1 or lower. Version 7.2 is the baseline to which the cluster can downgrade. 

## Breaking Changes 71CU3

| Versions | Area | Description | 
|-|-|-|
| **Windows - 7.1.456.9590<br>Ubuntu 16 - 7.1.410.1<br>Ubuntu 18 - 7.1.452.1804** | .Net Runtime EOL | Dotnet core LTS 2.1 runtime will go out of support from Aug 21, 2021. Service Fabric release after that date will drop support for dotnet core 2.1 SF apps. Service Fabric SDK will take a dependency on dotnet 3.* features to support SF dotnet core SDK.

## Breaking Changes 71

| Versions | Area | Description | 
|-|-|-|
| **Windows - 7.1.409.9590<br>Ubuntu - 7.1.410.1** | .Net Runtime | Service Fabric 7.2 and higher runtime drops support for .NET Core Service Fabric apps running with .NET Core 2.2 runtime. .NET Core runtime 2.2 is out of support from Dec 2019. Service Fabric runtime will not install .NET Core runtime 2.2 as part of its dependency. Customers should upgrade their .NET 2.2 runtime SF apps to the next .NET Core LTS version 3.1.
| **Windows - 7.1.409.9590<br>Ubuntu - 7.1.410.1** | Guest Executables and Containers | Guest executable and container applications created or upgraded in SF clusters with runtime versions 7.1+ are incompatible with prior SF runtime versions (e.g. SF 7.0).<br> 
Following scenarios are impacted: <br> <ul><li>An application with guest executables or containers is created or upgraded in an SF 7.1+ cluster. 
The cluster is then downgraded to a previous SF runtime version (e.g. SF 7.0). 
The application fails to activate.</li><li>A cluster upgrade from pre-SF 7.1 version to SF 7.1+ version is in progress.In parallel with the SF runtime upgrade, an application with guest executables or containers is created or upgraded.The SF runtime upgrade starts rolling back (due to any reason) to the pre-SF 7.1 version.The application fails to activate.<li></ul>To avoid issues when upgrading from a pre-SF 7.1 runtime version to an SF 7.1+ runtime version, do not create or upgrade applications with guest executables or containers while the SF runtime upgrade is in progress.<br><ul><li>The simplest mitigation, when possible, is to delete and recreate the application in SF 7.0.</li><li>The other option is to upgrade the application in SF 7.0 (for example, with a version only change).</li></ul><br>If the application is stuck in rollback, the rollback has to be first completed before the application can be upgraded again.

## Breaking Changes 70CU4

| Versions | Area | Description | 
|-|-|-|
| **Windows - 7.0.470.959<br>Ubuntu - 7.0.469.1** | Security configuration | Service Fabric versions below the Service Fabric 7.0 Third Refresh Release (Windows:7.0.466.9590, Linux:7.0.465.1) must first update to the 7.0 Third Refresh before upgrading to newer versions if the cluster has settings as depicted below. 
 
<!-- When no thumbprint is set up for primary account, the cluster must upgrade to 7.0 Third Refresh Release first. --> 
<Section Name="Security">
  <Parameter Name="ServerAuthCredentialType" Value="X509" /> <!-- Value other than None -->
</Section>
<Section Name="FileStoreService">
  <Parameter Name="PrimaryAccountNTLMX509Thumbprint" Value="" /> <!-- Empty or not set. -->
  <Parameter Name="CommonName1Ntlmx509CommonName" Value="" /> <!-- Empty or not set. -->
</Section> 

## Breaking Changes 70

| Versions | Area | Description | 
|-|-|-|
| **Windows - 7.0.457.9590<br>Ubuntu - 7.0.457.1** | Security configuration | For customers [using service fabric to export certificates into their linux containers](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-securing-containers), the export mechanism will be changed in an upcoming CU to encrypt the private key included in the .pem file, with the password being stored in an adjacent .key file 
| **Windows - 7.0.457.9590<br>Ubuntu - 7.0.457.1** | Managed identity | Starting in Service Fabric 7.0 version, customers using service fabric managed identities, please switch to new environment variables ‘IDENTITY_ENDPOINT’ and ‘IDENTITY_HEADER’. The prior environment variables 'MSI_ENDPOINT', ‘MSI_ENDPOINT_’ and 'MSI_SECRET' are now deprecated and will be removed in the next CU release. 
| **Windows - 7.0.457.9590<br>Ubuntu - 7.0.457.1** | Placement and Load Balancing | Starting with this release, we have changed the default value for the PreferUpgradedUDs config in the [Placement and Load Balancing section of Cluster Manifest](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-cluster-fabric-settings#placementandloadbalancing) .Based on numerous cases and customer issues, this default behavior should allow clusters to remain more balanced during upgrades. If you want to preserve today’s behavior for your workload, please perform a cluster configuration upgrade to explicitly set the value to true either prior to or as a part of upgrading your cluster to 7.0.
| **Windows - 7.0.457.9590<br>Ubuntu - 7.0.457.1** | Placement and Load Balancing | The calculation bug in the Cluster Resource Manager impacts how node resource capacities are calculated in cases where a user manually provides the values for node resource capacities. The impact of this fix is that the Cluster Resource Manager will now consider there to be less usable capacity in the cluster. More details in the [Resource governance documentation page](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-resource-governance).  
| **Windows - 7.0.457.9590<br>Ubuntu - 7.0.457.1** | Windows Azure Diagnostics | For customers using [Windows Azure Diagnostics](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-diagnostics-event-aggregation-wad) to collect Reliable Collections events, the Reliable Collections ETW manifest provider has been changed to 02d06793-efeb-48c8-8f7f-09713309a810, and should be accounted for in the WAD config. 
| **Windows - 7.0.457.9590<br>Ubuntu - 7.0.457.1** | Service Fabric SDK | Currently Service Fabric ships the following nuget packages as a part of our ASP.Net Integration and support:<br><ul><li>Microsoft.ServiceFabric.AspNetCore.Abstractions </li><li>Microsoft.ServiceFabric.AspNetCore.Configuration</li><li>Microsoft.ServiceFabric.AspNetCore.Kestrel</li><li>Microsoft.ServiceFabric.AspNetCore.HttpSys</li><li>Microsoft.ServiceFabric.AspNetCore.WebListener </li></ul>
These packages are built against AspNetCore 1.0.0 binaries which have gone out of support here: DotnetCore support . Starting with Service Fabric 8.x we will start building Service Fabric AspNetCore integration against AspNetCore 2.1 and for netstandard 2.0. As a result, there will be the following changes:<ul><li>The following binaries and their nuget packages will be released for netstandard 2.0 only.These packages can be used in applications targeting .net framework <4.6.1 and .net core >=2.0 - Microsoft.ServiceFabric.AspNetCore.Abstractions - Microsoft.ServiceFabric.AspNetCore.Configuration - Microsoft.ServiceFabric.AspNetCore.Kestrel - Microsoft.ServiceFabric.AspNetCore.HttpSys </li><li>The following package will no longer be shipped: - Microsoft.ServiceFabric.AspNetCore.WebListener: - Use Microsoft.ServiceFabric.AspNetCore.HttpSys instead. </li></ul>

## Breaking Changes 65CU2

| Versions | Area | Description | 
|-|-|-|
| **Windows - 6.5.658.9590<br>Ubuntu - 6.5.460.1** | Service Endpoint static port | Starting release 65CU2, if you specify a static port and assign it within the application port range resulting in a conflict, a Health Warning will be issued. The application deployment will continue in sync with current 6.5 behavior. However, in future major releases this application deployment will be prevented. For more information, please check the Warning section here: [Describing Azure Service Fabric apps and services - Azure Service Fabric | Microsoft Learn](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-application-and-service-manifests#describe-a-service-in-servicemanifestxml)  

## Breaking Changes 65

| Versions | Area | Description | 
|-|-|-|
| **Windows - 6.5.639.9590<br>Ubuntu - 6.5.435.1** | Visual Studio tools | This is the last release where we are releasing the Visual Studio tools for Service Fabric for VS 2015. Customers are advised to move to VS 2019. 
| **Windows - 6.5.639.9590<br>Ubuntu - 6.5.435.1** | Resource Governance | Protecting system entities from runaway 'user' code: From this version, users can now set up resource protection between the system and user services on a node. Service Fabric will enforce these hard resource limits for user services to guarantee that all the non-system services on a node will never use more than the specified number of resources. Please refer to this documentation: [Enforcing resource limits for user services](https://learn.microsoft.com/en-us/azure/service-fabric/service-fabric-resource-governance#enforcing-the-resource-limits-for-user-services) 

