# How to contribute to Service Fabric
Welcome! If you are interested in contributing to Service Fabric, reporting issues, or just getting in touch with the folks who work on Service Fabric, this guide is for you.

This repository is used for Service Fabric release notes and issue tracking. You can contribute by reporting non-production issues, suggesting features, participating in discussions, or submitting corrections to release notes and repository documentation. It does not contain the Service Fabric runtime source code.

## Open an Issue

### Production Issues
Issues related to production workloads **MUST NOT BE REPORTED** in this repository. Production support is out of scope of this repository's objectives. To receive product support, file a support request through the recommended channels below.
  
  #### Support Channels
  - [Azure Support options](https://azure.microsoft.com/en-us/support/options/)
  - [Service Fabric Support Policies](https://learn.microsoft.com/azure/service-fabric/service-fabric-support)

### Reporting security issues and bugs
Security vulnerabilities must not be reported through public GitHub issues. Follow the [security policy](SECURITY.md) to report them privately to the Microsoft Security Response Center (MSRC).

### Non-Production/Non-Security Issues

#### Guidelines for filing issues in this repository
   - File Service Fabric runtime bugs using the [bug report template](https://github.com/microsoft/service-fabric/issues/new?template=bug_report.md).
   - Suggest features using the [feature request template](https://github.com/microsoft/service-fabric/issues/new?template=feature_request.md).
   - Edit the title and fill in the details (runtime version, OS type, behaviors) requested in the template.
   - See [issue guidance](issue_guidance.md) for information about triage and the issue lifecycle.
     
 #### GitHub Repositories

- [Microsoft/service-fabric](https://github.com/microsoft/service-fabric) (this repository) for bugs or feature requests related to **Service Fabric Runtime**. When filing an issue, please tag **@microsoft/service-fabric-triage** team to get the issue triaged.
- [Microsoft/service-fabric-services-and-actors-dotnet](https://github.com/Microsoft/service-fabric-services-and-actors-dotnet) for issues related to **Reliable Services, Reliable Actors, and Service Remoting**.
- [Microsoft/service-fabric-aspnetcore](https://github.com/Microsoft/service-fabric-aspnetcore) for issues related to **ASP.NET Core integration with Reliable Services**.
- [Microsoft/service-fabric-cli](https://github.com/Microsoft/service-fabric-cli) for any issues related to the **SF CLI (sfctl)**.
- [Microsoft/service-fabric-explorer](https://github.com/Microsoft/service-fabric-explorer) for issues related to **Service Fabric Explorer**.

- [RCMissingTypes tools previewed in 7.0](https://github.com/hiadusum/ReliableCollectionsMissingTypesTool)
- [Backupexplorer](https://github.com/microsoft/service-fabric-backup-explorer) for issues related to **Service Fabric Backup Explorer**.
- [POA](https://github.com/microsoft/Service-Fabric-POA) for issues related to **Patch Orcherstration Agent**.
- [Auto Scale Helper](https://github.com/Azure/service-fabric-autoscale-helper)
- [Application DR Tool](https://github.com/microsoft/Service-Fabric-AppDRTool) for issues related to **Service Fabric Application Disaster Recovery Tool**.
- [FabricObserver/ClusterObserver](https://github.com/microsoft/service-fabric-observer) for issues related to **Service Fabric Observer**.


### Other discussions
For general "how-to" and guidance questions about using Service Fabric to build and run applications, please use [Stack Overflow](http://stackoverflow.com/questions/tagged/azure-service-fabric) tagged with `azure-service-fabric`.

GitHub supports [markdown](https://help.github.com/categories/writing-on-github/), so when filing bugs make sure you check the formatting before clicking submit.

## Contributing release notes and documentation
We welcome corrections and improvements to release notes, issue guidance, and repository documentation. For source-code contributions to other Service Fabric components, use the relevant component repository listed above.

### Communication
 - Before making a substantial change to release notes or repository documentation, please open an issue to discuss the proposed change. For small corrections, you can open a pull request directly.

### Development process
Please be sure to follow the usual process for submitting PRs:

 - Fork the repo
 - Create a pull request
 - Make sure your PR title is descriptive
 - Include a link back to an open issue in the PR description

We reserve the right to close PRs that are not making progress. If no changes are made for 7 days, we'll close the PR. Closed PRs can be reopened again later and work can resume.

### Contributor License Agreement
Before you submit a pull request, a bot will prompt you to sign the [Microsoft Contributor License Agreement](https://cla.microsoft.com/). This needs to be done only once for any Microsoft-sponsored open source project - if you've signed the Microsoft CLA for any project sponsored by Microsoft, then you are good to go for all the repos sponsored by Microsoft.

 > **Important**: Note that there are **two** different CLAs commonly used by Microsoft projects: [Microsoft CLA](https://cla.microsoft.com/) and [.NET Foundation CLA](https://cla2.dotnetfoundation.org/). Service Fabric open source projects use the [Microsoft](https://cla.microsoft.com/) CLA. The .NET Foundation is treated as a separate entity, so if you've signed the .NET Foundation CLA in the past, you will still need to sign the Microsoft CLA to contribute to Service Fabric open source projects.
