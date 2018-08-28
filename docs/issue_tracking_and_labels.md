# GitHub issue tracking and labels

Describes how to use labels and modifiers when tracking work items in the GitHub page.

## Assignees

No labels for this section - just use the alias of the individuals owning the issue.

## Labels

All tags and labels are lower-case and dash separated.

|      Status      |           Area           | Issue Type  | Priority |
| ---------------- | ------------------------ | ----------- | -------- |
| investigating    | area-common              | question    | pri-0    |
| needs-more-info  | areas-communication      | enhancement |          |
| understood       | area-data-structures     | code-defect |          |
| in-design        | area-developer           | test-defect |          |
| in-progress      | area-diagnostics         | doc-only    |          |
| external         | area-documentation       |             |          |
| release-note-na  | area-engineering         |             |          |
| release-note-req | area-federation          |             |          |
| duplicate        | area-hosting             |             |          |
|                  | area-ktl                 |             |          |
|                  | area-linux               |             |          |
|                  | area-management          |             |          |
|                  | area-operations          |             |          |
|                  | area-reliability         |             |          |
|                  | area-security            |             |          |
|                  | area-setup               |             |          |
|                  | area-sfrp                |             |          |
|                  | area-standaloneinstaller |             |          |
|                  | area-store               |             |          |
|                  | area-testinfra           |             |          |
|                  | area-testability         |             |          |
|                  | area-transport           |             |          |
|                  | area-needs-triage        |             |          |
|                  | area-try-service-fabric  |             |          |
|                  | area-windows-server      |             |          |
|                  |                          |             |          |

For more information about the definition of each label, take a look at the
[labels page](https://github.com/Microsoft/service-fabric/labels) on GitHub.

There are no strict rules about using labels, apply labels when appropriate
according to each label's definition. Generally labels fall into one of 4
categories

**Status**: Indicates the work status or issue status, including if the issue
has been seen by a developer, been investigated, or needs more attention. It
can also be used to indicate an outcome of an issue

**Area**: Indicates the impacted component. These are modules and feature
areas of Service Fabric.

**Issue Type**: Indicates what type of work is required to resolve the issue.
Defects require code changes, questions or doc changes can be resolved without
changing production code.

**Priority**: Indicates how soon an issue will get attention. Avoid adding
any priory labels unless there is a strong justification.

## Milestones

Releases are defined by milestones, these are usually denoted by a sequence of
two digits and a possible post-release fix identifier. Examples of this are:

- 6.3
- 6.4 CU1

In this case 6.4 CU1 refers to the 6.4 cumulative update 1. This would
succeed the 6.4 release in the case there were fixes to that released version.

If no milestone is added, the item is considered to be on the backlog

## Example label transitions

Issues are reported with no labels, and no assignees. Issues then typically
follow this cycle:

### When an issue arrives

- Add an area, if not certain, add `needs-triage`
- Add an issue type, if not certain, ignore

### When starting working on an issue

- Add an assignee, whoever is actively working on the issue
- Update issue status according how far along the investigation is
- Adjust area if it needs adjusting
- If the issue needs to be transferred to a new area, be sure to remove the current assignee first

### When closing issues

- Make sure the area and type are set correctly

### Assigning issues

Issues should only be assigned to individuals if they are actively being
worked on by that individual. Otherwise, issues remain unassigned. This way it
is obvious who is responsible for the work.

## Goals and Responsibilities

In general, GitHub is not meant to replace live support. If you have an issue
that requires immediate attention please use [Azure Support](https://azure.microsoft.com/en-us/support/options/).

### Contributors

If you are an active contributor and own a section of the code, it is expected
that issues assigned to you are actively being worked on.

Responses should take less than 48 hours.

### Area lead

If you are responsible for an area, it is expected you triage all issues assigned
to that area, and validate they are closed correctly.

Triaging issues involves setting correct labels and milestones, as well as driving
to closure.
