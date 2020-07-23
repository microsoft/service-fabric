# How to contribute to Service Fabric
Welcome! If you are interested in contributing to Service Fabric, reporting issues, or just getting in touch with the folks who work on Service Fabric, this guide is for you.

One of the easiest ways to contribute is to participate in discussions and discuss issues. You can also contribute by opening an issue and submitting a pull request with code changes.

## Open an Issue

### Production Issues
Issues related to Production workloads  **MUST NOT BE REPORTED** in this repository. Production support is out of scope of this repository's obejctives. To receive product  support, you must file a support request through below recommended channels.
  
  #### Support Channels
  - [Azure Support options](https://azure.microsoft.com/en-us/support/options/)
  - [Service Fabric Support Policies](https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-support) 

### Reporting security issues and bugs
Security issues and bugs should be reported privately, via email, to  **[Microsoft Security Response Center (MSRC)](secure@microsoft.com)**. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://technet.microsoft.com/en-us/security/ff852094.aspx).

### Non-Production/Non-Security Issues

#### Guidelines to files issues for this repository
   -  File issues pertaining to Service Fabric runtime in this repo using this [template](https://github.com/microsoft/service-fabric/issues/new?template=create_new_issue.md) 
   -  Edit the title and fill in the details (runtime version, OS type, behaviors) requested in the template.
   -  New issues will be assigned automatically to our team @microsoft/service-fabric-triage . ***Do not update the assignees listed in the template***
     
 #### GitHub Repositories

- [Microsoft/service-fabric](http://github.com/Microsoft/service-fabric) (this repository) for bugs or feature requests related to **Service Fabric Runtime**. When filing an issue, please tag **@microsoft/service-fabric-triage** team to get the issue triaged.
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

## Contributing code and content
We welcome all forms of contributions from the community. Please read the following guidelines  to maximize the chances of your PR being merged.

### Communication
 - Before starting work on a feature, please open an issue on GitHub describing the proposed feature. As we're still getting our development process off the ground, we want to make sure any feature work goes smoothly. We're happy to work with you to determine if it fits the current project direction and make sure no one else is already working on it.

 - For any work related to setting up build, test, and CI for Service Fabric on GitHub, or for small patches or bug fixes, please open an issue for tracking purposes, but we generally don't need a discussion prior to opening a PR.

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
