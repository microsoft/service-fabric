# Microsoft Azure Service Fabric 9.0 Cumulative Update 2.1 Release Notes

This release includes a fix for Linux only as described below in this document.

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu 18 <br> Ubuntu 20 | 9.0.1086.1 <br> 9.0.1086.1 |


## Contents 

Microsoft Azure Service Fabric 9.0 Cumulative Update 2.1 Release Notes

* [Current Breaking Changes](#current-breaking-changes)
* [Retirement and Deprecation Path Callouts](#retirement-and-deprecation-path-callouts)
* [Repositories and Download Links](#repositories-and-download-links)


## Current Breaking Changes
Customers running Azure Service Fabric Linux Clusters will not be able to scale out or reimage (manual or auto via VMSS OS Image Update) nodes or create new Linux clusters without upgrading to this version of Service Fabric (9.0.1086.1). Also, if your application requires JDK, then you will need to install a JDK of your choice since SF will not install the JDK as part of the SF installation.
Starting 8/19/2022, the following versions will be deprecated and will not be available on Linux for new cluster creation or node reimage/scale out operations:<br>
* 9.0 CU2/CU1/RTO
* 8.2 CU4/CU3/CU2.1/CU2/CU1/RTO<br>


**Details:** Service Fabric Runtime (for Linux) placed a dependency on Zulu for Azure build of OpenJDK which is no longer supported as announced in their dev [blog](https://devblogs.microsoft.com/java/end-of-updates-support-and-availability-of-zulu-for-azure/). Since the creation of new Linux clusters or scale-out/reimage of nodes required all the dependencies to be installed, these workflows broke when the dependency became unavailable.<br>
Beginning with this release, Service Fabric Runtime will no longer install any JDK dependency which will ensure that customer can scale out, reimage, or create new Linux clusters. If your application needs JDK, please utilize an alternate mechanism to provision a JDK. To get the latest Service Fabric Runtime releases with this change, follow the upgrade documentation. If you depend on a JDK, please be sure to install one of your choice.<br>
For more information see: [Breaking change for Azure Service Fabric Linux customers](https://techcommunity.microsoft.com/t5/azure-service-fabric-blog/breaking-change-for-azure-service-fabric-linux-customers/ba-p/3604678)

## Retirement and Deprecation Path Callouts
* Service Fabric ASP.NET Core packages built against ASP.NET Core 1.0 binaries are out of support. Starting Service Fabric 8.2, we will be building Service Fabric ASP.NET Core packages against .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, .NET Framework 4.6.1. As a result, they can be used only to build services targeting .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, >=.NET Framework 4.6.1 respectively.
For .NET Core 3.1, .NET Core 5.0, .NET Core 6.0, Service Fabric ASP.NET Core will be taking dependency on Microsoft.AspNetCore.App shared framework; whereas, for NetFx target frameworks >=.NET Framework 4.6.1, Service Fabric ASP.NET Core will be taking dependency on ASP.NET Core 2.1 packages.
The package Microsoft.ServiceFabric.AspNetCore.WebListener will no longer be shipped, Microsoft.ServiceFabric.AspNetCore.HttpSys package should be used instead.
* .NET 5.0 runtime has reached end-of-life on May 8, 2022. Service Fabric releases after this date will drop support for Service Fabric applications running with .NET 5.0 runtime. Current applications running on .NET 5.0 runtime will continue to work, but requests for investigations or request for changes will no longer be entertained. Please migrate to using .NET 6.0 version instead.
* Ubuntu 16.04 LTS reached its 5-year end-of-life window on April 30, 2021. Service Fabric runtime has dropped support for 16.04 LTS, and we recommend moving your clusters and applications to Ubuntu 18.04 LTS or 20.04 LTS. Current applications running on it will continue to work, but requests to investigate issues w.r.t this OS or requests for change will no longer be entertained. We have stopped producing Service Fabric builds for U16.04 so customers will no longer get SF related patches. Please migrate to Ubuntu 18.04 or 20.04 instead.
* Service Fabric runtime will soon stop supporting BinaryFormatter based remoting exception serialization by default and move to using Data Contract Serilization (DCS) based remoting exception serialization by default. Current applications using it will continue to work as-is, but Service Fabric strongly recommends moving to using Data Contract Serilization (DCS) based remoting exception instead.
* Service Fabric runtime will soon be archiving and removing Service Fabric runtime version 6.4 packages and older, as well as SDK version 3.3 packages and older from the package Download Center. Archiving/Removing will affect application scaling and re-imaging of virtual machines in a Service Fabric Cluster running on unsupported versions. After older versions are removed/archived, this may cause failure while rolling back when the current in-progress upgrade has errors. 

## Repositories and Download Links
The table below is an overview of the direct links to the packages associated with this release. 
Follow this guidance for setting up your developer environment: 
* [Geting Started with Linux](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-linux)
* [Getting Started with Mac](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started-mac)
* [Getting Started with Windows](https://docs.microsoft.com/azure/service-fabric/service-fabric-get-started)

| Area | Package | Version | Repository | Direct Download Link |
|-|-|-|-|-|
|Service Fabric Runtime |Ubuntu Developer Set-up | 9.0.1086.1 |N/A | Cluster Runtime: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabric <br> Service Fabric SDK for local cluster setup: https://apt-mo.trafficmanager.net/repos/servicefabric/pool/main/s/servicefabricsdkcommon/ <br> Container image: https://hub.docker.com/r/microsoft/service-fabric-onebox/ 

