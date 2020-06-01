# Microsoft Azure Service Fabric 7.1 First Refresh Release Notes

This release includes bug fixes and features as described in this document. 

The following packages and versions are part of this release:

| Service | Platform | Version |
|---------|----------|---------|
|Service Fabric Runtime| Ubuntu <br> Windows |  <br>  |
|Service Fabric for Windows Server|Service Fabric Standalone Installer Package | |
|.NET SDK |Windows .NET SDK <br> Microsoft.ServiceFabric <br> Reliable Services and Reliable Actors <br> ASP.NET Core Service Fabric integration| <br> <br> <br> |
|Java SDK  |Java for Linux SDK  |  |
|Service Fabric PowerShell and CLI | AzureRM PowerShell Module  <br> SFCTL |  0.3.15 <br> 9.0.0 |

## Contents 

Microsoft Azure Service Fabric 7.1 First Refresh Release Notes

* [Key Announcements](#key-announcements)
* [Service Fabric Runtime](#service-fabric-runtime)
* [Repositories and Download Links](#repositories-and-download-links)

## Key Announcements

* [**General Availability of Request Drain**](https://docs.microsoft.com/azure/service-fabric/service-fabric-application-upgrade-advanced#avoid-connection-drops-during-planned-downtime-of-stateless-services): During planned service maintenance, such as service upgrades or node deactivation, you would like to allow the services to gracefully drain connections. This feature adds an instance close delay duration in the service configuration. During planned operations, SF will remove the Service's address from discovery and then wait this duration before shutting down the service.
* [**General Availability of Ephemeral Disks**]



## Service Fabric Common Bug Fixes

| Versions | IssueType | Description | Resolution | 
|-|-|-|-|
| **Windows  7.1.416.9590** | **Bug** |Central Secret Service does not handle transaction failures|**Brief desc**:  Prior to SF71CU1, the Central Secure Store service could fail on concurrent transactions, potentially leaving stored entities in an inconsistent state.  <br> **Impact**: Secrets with concomitant PUTs and DELETEs may silently fail, leaving the secret in an inconsistent state. <br> **Workaround**: Avoid parallel opposing (PUT/DELETE) operations on the same entity. <br> **Fix**:  CSS enforces uniqueness checks and rolls back failed transactions.
| **Ubuntu 7.1.418.1** | **Bug** |Security setting validator fails incorrectly on Linux|**Brief desc**:  The SF functionality which attempts to pre-validate security settings, during an initial cluster deployment or upon changing its security settings, incorrectly marked cluster certificates as non-existent on Linux.   <br> **Impact**: Setting the Security\EnforcePrevalidationOnSecurityChanges setting to true on Linux will cause cluster upgrades/deployments to fail. <br> **Workaround**: Do not enable this setting (by default it is disabled). <br> **Fix**:  Disable the setting (its default value is false)
| **Windows  7.1.416.9590** | **Bug** |Exclude app endpoint certs declared by TP from monitoring|**Brief desc**:   SF71 introduced support for declaring app endpoint certs by subject common name; to enable auto-rollover, these certificates must be monitored for renewal and ACL’d accordingly. Prior SF71, endpoint certificates had been declared by thumbprint only, and were not ACLd explicitly by SF code. Instead, the SF runtime would look up the certificate by thumbprint, ACL the endpoint port and then bind the cert to the port. After SF71, all endpoint certificates are pre-ACLd as part of deploying an application to a node, which can cause a failure of application activation or upgrade in the following scenarios: <br>**(a)** Applications which used certs not supported by SF but supported by http.sys: Elliptic Curve certs, or certs with private keys managed by CAPI2 (CNG certs) </br>**(b)** Applications with multiple services and multiple endpoint certificates, which used placement restrictions to separate the services among node types, and did not provision all certificates to all nodes.<br> **Impact**:in either of the scenarios above, application activation will fail. This may happen in the normal course of operation, or during a cluster or application upgrade. In that case, the upgrades will roll back. <br> **Workaround**:  for scenario (a) above, there is no feasible workaround. If possible, though, consider using RSA certificates with private keys managed by a CSP (CryptoServiceProvider, CAPI1). If this is not possible, please do not upgrade to SF71, go directly to SF71CU1. For scenario (b) above, install all declared endpoint certificates on all nodes where the application is deployed, irrespective of the segregation of the services comprising the application. <br> **Fix**: SF71CU1 exempts endpoint certificates declared by thumbprint from explicit/required ACLing as part of app activation - in essence it restores the pre-SF71 behavior.
| **Windows  7.1.416.9590** | **Bug** |Upload files to ImageStore creates directory marker files in current directory|**Brief desc**:  Registering application (a.k.a. Provision) creates temporary files in current directory. The access to current directory may be impacted by runtime environment. The client's process might change the current directory during uploading or the client may not have write access to the running directory. <br> **Impact**: Uploading or registering application package could fail by errors like ACCESSDENIED or FileNotFound <br> **Workaround**: A client that uses FabricClient ensures that the write access is granted to the process through uploading and registering application package. <br> **Fix**: The working directory is changed from current directory to process's temporary directory.
| **Windows  7.1.416.9590** | **Bug** |ImageStoreService primary recovers only one file among all missing files|**Brief desc**:ImageStoreService doesn't recover every file when new primary replica detects missing files in storage. The ImageStoreService becomes healthy with missing files. <br> **Impact**: Application will fail to activate replica on nodes that can't download required files. <br> **Workaround**: A client that uses FabricClient ensures that the write access is granted to the process through uploading and registering application package. <br> **Fix**: ImageStoreService primary copies every file from secondary replica if any missing file in file system is detected. 
| **Windows  7.1.416.9590** | **Bug** |InstallFabricRuntime: UAC doesn't appear to grant all necessary privileges for install|**Brief desc**:Users launching the SF runtime installer were hitting a jarring experience where the launcher would start and immediately close. This is because it is intended to be launched with an /accepteula parameter and since the executable would launch in a separate User Account Control (UAC) console it would close before the user had a chance to understand the intended usage pattern in the exit message. <br> **Impact**: Users installing SF runtime for as part of SDK dev machine setup, outside of the WebPI experience. <br> **Workaround**: Run the runtime installer exe from administrator cmd console. This will self-describe the required parameters. <br> **Fix**: We are now detecting when the executable is launched in a way where the program output would not get a chance to be read, and waiting for first of 1) user input or 2) 30 seconds before closing.



 