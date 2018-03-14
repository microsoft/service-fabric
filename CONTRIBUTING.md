# How to contribute to Service Fabric
Welcome! If you are interested in contributing to Service Fabric, reporting issues, or just getting in touch with the folks who work on Service Fabric, this guide is for you.

One of the easiest ways to contribute is to participate in discussions and discuss issues. You can also contribute by opening an issue and submitting a pull request with code changes.

## Where to go
Service Fabric is currently in the middle of a big transition to open development on GitHub. Please bear with us during this time as we refine the contribution process and get Service Fabric situated in its new home. 

In the meantime, follow this guide to get to the right place depending on what you're looking to do.

### Help Service Fabric move to GitHub
If you're interested in helping us move (or you just like working on build and test automation), you're in the right place. Head over to the [issues](https://github.com/Microsoft/service-fabric/issues) list where we are currently tracking work related to moving Service Fabric to open development.

### Open an issue
For non-security related bugs, issues, or other discussion, please log a new issue in the appropriate GitHub repo:

- [Azure/service-fabric-services-and-actors-dotnet](https://github.com/Azure/service-fabric-services-and-actors-dotnet) for issues related to **Reliable Services, Reliable Actors, and Service Remoting**.
- [Azure/service-fabric-aspnetcore](https://github.com/Azure/service-fabric-aspnetcore) for issues related to **ASP.NET Core integration with Reliable Services**.
- [Azure/service-fabric-cli](https://github.com/Azure/service-fabric-cli) for any issues related to the **SF CLI (sfctl)**.
- [Azure/service-fabric-explorer](https://github.com/Azure/service-fabric-explorer) for issues related to **Service Fabric Explorer**.
- [Azure/service-fabric-issues](http://github.com/Azure/service-fabric-issues) for all other bugs or feature requests related to **Service Fabric in general**. Note that we will eventually migrate the issues repo into the main project repo, but for now we'd like to keep these separated from issues related to the transitioning of Service Fabric onto Github.
- [Microsoft/service-fabric](http://github.com/Microsoft/service-fabric) for any issues specific to problems with **contributing to Service Fabric**, for example Docker builds not working, trouble running tests, etc.

### Reporting security issues and bugs
Security issues and bugs should be reported privately, via email, to the Microsoft Security Response Center (MSRC)  secure@microsoft.com. You should receive a response within 24 hours. If for some reason you do not, please follow up via email to ensure we received your original message. Further information, including the MSRC PGP key, can be found in the [Security TechCenter](https://technet.microsoft.com/en-us/security/ff852094.aspx).

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
