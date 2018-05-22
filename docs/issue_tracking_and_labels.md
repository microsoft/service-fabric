Best Practices Guide for GitHub usage
====================================
>
Assignees
---------------------------------------------------------------

> No labels for this section - just use the alias of the individuals owning the issue.

Labels
------------------------------------------------------------

> It is understood that some teams will have their own GitHub page. Focus is on the SF page and we can build on/modify for other Team GitHub pages later once the initial model is completed.
>
>  
### Label Rules for GitHub

-   All tags and labels are lower-case and dash separated
>  

| ***Status*** | ***Area***               | ***Issue Type*** | ***Pri***       |
|--------------------|--------------------------|------------------|-----------------|
| investigating      | area-common              | question         | pri-0, |
| needs-more-info    | areas-communication      | enhancement      |                 |      
| understood         | area-data-structures     | code-defect      |                 |
| in-design          | area-developer           | test-defect      |                 |
| in-progress        | area-diagnostics         | doc-only         |                 |
| external           | area-documentation       |                  |                 | 
| release-note-na    | area-engineering         |                  |                 |
| release-note-req   | area-federation          |                  |                 |
| duplicate          | area-hosting             |                  |                 |
|                    | area-ktl                 |                  |                 |
|                    | area-linux               |                  |                 |
|                    | area-management          |                  |                 |
|                    | area-operations          |                  |                 |
|                    | area-reliability         |                  |                 |
|                    | area-security            |                  |                 |
|                    | area-setup               |                  |                 |
|                    | area-sfrp                |                  |                 |
|                    | area-standaloneinstaller |                  |                 |
|                    | area-store               |                  |                 |
|                    | area-testinfra           |                  |                 |
|                    | area-testability         |                  |                 |
|                    | area-transport           |                  |                 |
|                    | area-needs-triage        |                  |                 |
|                    | area-try-service-fabric  |                  |                 |
|                    | area-windows-server      |                  |                 |
|                    |                          |                  |                 |


--------------------------------------------------------------------------


----------------------------------------------------------------

| ***Release***                                                            |
|-------------------------------------------------------------------------------------------|
| 6.3, 6.4, etc. (define specifically once we get a build number at end of release)         
                                                                                            
 6.3 CU1, 6.4 CU1, etc. (define specifically once we a get build number at end of release)  
                                                                                            
 > Example 6.3.xxx.xx                                                                       
 >                                                                                          
 > All work items update automatically once the milestone is updated. (= minimal effort)    
                                                                                            
 There is not a vNext or backlog release.                                        
                                                                                            
 > No Milestone = backlog                                                                   |

Happy Path By issue
===================

Path below based on current State/Phase Labels

****Question Happy Path****

*Open -&gt; triaged -&gt; needs-more-info (optional) -&gt; in-progress -&gt; Closed. *

****Bug or Documentation Happy Path****

*Open -&gt; triaged -&gt; investigating (optional) -&gt; need-more-info (optional) -&gt; understood (optional) -&gt; in -progress -&gt; Closed. *

****Enhancement Happy Path****

*Open -&gt; triaged -&gt; investigating (optional) -&gt; design -&gt; in -progress -&gt; review -&gt; Closed. *

> ****Questions, Defects and Doc issues****
>
> ***'Optional' Phases will apply to issues that are immediately understood allowing them to bypass many of the phases, moving the bug right to 'in-progress'.***

****Enhancement Issues****

> ***Where at 'review' the feature is reviewed for release and obtains any special sign-off needed from feature teams (if applicable). If not done (completed or has issues), go back to investigate (if needed) or move to design or in-progress for additional work. Repeat as needed. Once the feature can pass the review process (Whatever that may be for the given feature) it moves to closed***

Issue Lifecycle
===============

Issues are reported with no labels, and no assignees. Issues then typically follow this cycle:

1.  When an issue is reported it is sent to an appropriate area using the area label

2.  If the type of issue is obvious, a type label is set

3.  If the issue is not obvious, an investigating status label is set, and assignee is set

4.  The individual assigned works on the issue, updating the issue, status and type labels

If the assignee has found the issue does not belong to their area, they initiate a transfer by

1.  Removing the area label

2.  Removing themselves from assignee

3.  Transferring issue to new area label

Assigning issues
================

> Issues should only be assigned to individuals if they are actively being worked on by that individual. Otherwise, issues remain unassigned. This way it is obvious who is responsible for the work. It is valid for no one to be responsible.

Work Flow Process Details
=========================

Who are all those involved with the project?
--------------------------------------------

Inputs will come from both **Internal** and **External** service-fabric repo contributors and community members. Here are some of the roles we'll see.

| **Area Leads**:   | Responsible for setting direction in specific areas of the project and triaging issues within their respective areas.|
|-|-
| **Committers**:   | Dedicated to working on the project, may have write access.|
| **Contributors**: | Community members that make any contribution to the project, whether it’s submitting code, fixing or reporting bugs, writing docs, etc.|
| **Users:**        | General users who have a need for the project.|

> **As a measurement** we want to try and touch all new items within 48 hours. This is not exposed to the customer but is a metric for us to determine how quickly we are touching new issues. We need to ensure that feature area owners are actively triaging their issues and keeping them up to date.
>
> A defined group of individuals consisting of Area Leads or Developers will be responsible for reviewing all new items.
>
> Once a new item has arrived the defined resource for 'touching' these issues for the first time will set the label 'triage' to the issue so that it can be reviewed.
>
> Once the item is being reviewed, they will be assigned an 'Issue type' unless it requires additional investigation, then the 'investigation' label would be added to show there needs to be some additional work before routing the issue.

Question Issues
---------------

> Once marked as a 'Question' the resource will assist with the issue by pointing to self-service options or provide customized help to resolve the issue. We like to have questions actively pushed out to SO and slack (once we build up the community).
>  
> ****Questions****
>
> ***'Optional' Phases will apply to issues that are immediately understood allowing them to bypass many of the phases, moving the bug right to 'in-progress'.***
>
> If the issue is not clear, the resource changes the label to 'needs-more-info' so the person who created the issue understands there needs to be additional engagement by them in order to get help.
>
> **Best Practice!**
>
> If the question can be handled in a quick exchange, then there is no need to add the 'in progress label. If there is some research or it will take some time to resolve the issue the use the 'in progress label.
>
> At this point the resource just needs to complete the request and close out the question once completed. There is not a label for this state since 'closed' is its own unique state.
>
> If it is determined the 'Question' is actually a 'Bug' or 'Enhancement' the 'Question' label will be removed and updated accordingly and assigned (if necessary) to the appropriate resource.

Defects, Enhancement, Doc issues
--------------------------------

> After being 'triaged' and defined as either a ‘Code-Defect’, ‘Test-Defect’, or 'Enhancement' the item will need to be reviewed and have the following labels applied based on the review of the issue.  

-   The State Label will be changed to 'investigating or understood'

-   The Area Label will be added

-   The Milestone label will be added (As needed.)

-   The Priority Label will be supplied (As needed.)

> ****Defects and Doc issues****
>
> ***'Optional' Phases will apply to issues that are immediately understood allowing them to bypass many of the phases, moving the bug right to 'in-progress'.***

****Enhancement Issues****

> ***Where at 'review' the feature is reviewed for release and obtains any special sign-off needed from feature teams (if applicable). If not done (completed or has issues), go back to investigate (if needed) or move to design or in-progress for additional work. Repeat as needed. Once the feature can pass the review process (Whatever that may be for the given feature) it moves to closed***
>
> **Important! The Bug or Enhancement will remain un-assigned until someone is ready to pick up the work!** We do not want to just assign items to people who have work already since it is possible others may want to contribute to the fix/feature.
>
> At the very least the issue can be assigned a future Milestone if it is understood that is where the work needs to be done **OR** if there is not a clear path for the bug, leaving the Milestone blank functions as a backlog for issues without a release assigned. Once it reaches the current Milestone, the additional labels may be added.
>
> **Responsibility of the Area Owner**
>
> At this time, the Bug or Enhancement is **NOT** assigned to a specific individual, just the Area. Area owners need to be aware of the items in their queue and need to be aware of the Milestone so that they can review the active, unassigned items and be prepared to assign them once a resource is ready for work.
>
> Once the issue is ready to be worked on (either by resource of by Milestone deliverable) an individual will be assigned to do the work. The 'in-progress' label is applied now removing the 'understood' label').
>
> Work on the issue will continue until completed. i.e. the fix/feature is validated and approved by the Dev Lead and no regressions are associated with the issue. Once work is done and all necessary related tasks are completed, the item is closed.
>
> Specific build numbers will be used to update the Milestone label so we know specifically what build number shipped and what fixes are associated with it. Changing the label changes ALL the tagged issues at one time.
>
> Documentation can be taken from GitHub based on build number in the Milestone and consolidated for Release Notes.
>
> Once all work is completed and signed off on, we can RTW and close-down the Milestone and move any remaining items into the appropriate/next Milestone.

Work Flow Process Table (Same as above - fewer words)
=====================================================

| **State/Phase label**   | **Issue Type**             | **State description**                                                                                                                                                | **Action**                                                                                                                                                                                          |
|-------------------------|----------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **New/Open (no label)** | Question, Bug, Enhancement | No label when new issue type is created. |- A new issue is opened under the appropriate GitHub page. <br>-New item **must** be triaged in less than 48 hours!|
| **No label**            | Question, Bug, Enhancement | New issues should be triaged within 48 hours of being opened. Add the 'needs-triage' label at this time if not clear what area issue impacts                         |- Area PM/Dev Lead will triage the issue. <br>- Define whether it is a bug, Enhancement or Question.                                                             
| **investigating**       | Bug, Enhancement           | Potentially optional if the issue is not fully understood. Remove 'triage' and add 'investigation'. Issue which are understood can move right ahead to 'Understood'. |- Define the Area (Team). <br>- Define the Milestone (Iteration). <br>- Define Priority                                                                             
| **needs-more-info**     | Question, Bug              | If an item needs more information before it can be assigned use this label. This is also optional. |- It an issue needs more info then it is up to the current owners to reach out to get that info so that the item can be understood and ready to be picked up for work.<br> - Any sections not previous defined (Area, Owner, etc.) those should be added at this time.
| **understood**          | Bug                        | This label is assigned once the bug issue understood and ready to be assigned.|- This is a parked phase where the bug contents are understood and ready to be worked on once the defined resource is ready to engage. <br>- The necessary resources will be added to the 'Assignees' section of the issue. |
| **design**              | Enhancement                | Use this label to define the enhancement is being worked on and due to its nature requires more than just coding to address.|- Related design work, meetings and other activities rewalted to coding a new feature will occur during this phase. <BR> - A new Enhancement might be reviewed more than once and move back to a prior state before work resumes on the new enhancement. |
| **In-progress**         | Question, Bug, Enhancement | When the work is understood and actively being worked on.|- For a Bug, once the Dev resource engages the label will be changed to 'In-Progress'. <BR>- For Enhancements, the label of 'in-progress' is only added once the current design aspects have been defined fully.|
| **review**              | Enhancement                | Use this label when an enhancement is under review. This work item may need to go back to 'investigation' or 'design' based on the outcome of the review. |- This phase is unique to Enhancements. Once an enhancement has been completed, it needs to be reviewed prior to release to ensure all aspects of the new enhancement (feature) work as expected. |
| **closed**              | Question, Bug, Enhancement | No label is required here. Once all work on a given issue is completed the issue is closed.|- Completed issues will be closed and remain as such unless Recall mode requires it to be reverted. <br>- No label change is required so long as the item is marked 'Closed'. <br>- Items can be chosen by 'Milestone' and 'Closed' to get documentation for given release.<br>- Do we still need to make notifications about the release? <br>- No further action required for issue.|
