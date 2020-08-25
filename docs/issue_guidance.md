# Issue guidance

This document details how Service Fabric handles issues reported through GitHub.

**Note**, for support, guidance on opening issues, and general questions related to using Service Fabric, take a look
at the documentation in the [contributing guide](/CONTRIBUTING.md).

# Triaging issues

Issues are triaged using GitHub labels, the following labels are used to indicate the status
of the issue.

## Bug, enhancement and question

Most labels are used according the GitHub standards, these labels indicate the type of issue.

|     Label     |                                             Description                                             |
| ------------- | --------------------------------------------------------------------------------------------------- |
| investigating | The issue isn't clear, further investigation is needed to determine if this a bug or something else |
| bug           | The issue is related to a code or configuration defect in Service Fabric                            |
| enhancement   | The issue is a request to improve a certain feature of Service Fabric                               |
| question      | The issue is related to a clarification on how Service Fabric works                                 |


## Area and needs triage

Area labels and triage labels are used to identify the related involved components of Service Fabric.

|    Label     |                                Description                                |
| ------------ | ------------------------------------------------------------------------- |
| needs triage | The involved component isn't clear, further investigation is needed       |
| area/*       | Area labels indicate the specific component that is involved in the issue |


# Issue lifecycle

Issues are handled according to the set of labels applied to them. As issues progress from being
identified, and investigated, to being actively worked on, the set of labels will changes. 

Once confirmed, issues will have an area label and a `bug` or `enhancement` label.

## Assigning issues

Issues should not be assigned until an active developer is working on coding changes, or an owner
has been identified that will work to resolve the issue in reasonable time.

The assigned owner should also change as the area changes, to make sure issues end up routed
correctly.

## Expected response times

**Note**, If you are experiencing a Service Fabric issue and require immediate support, please
reach out to Azure Customer Support through the web portal. GitHub issues are not meant to replace
Azure Customer Support.

The Service Fabric team tries to respond quickly to GitHub issues. We also expect individuals who report
issues to respond when additional information is required. For minor questions and issues, it's best to 
reach a Service Fabric community member through [Slack]() or [Stack Overflow](https://stackoverflow.com/questions/tagged/azure-service-fabric)

For issues that are reported through GitHub, we try to adhere to the following time bounds:

- Initial acknowledgement of issue in less than 48hrs
- Stale or out of date issues closed after 1 week
