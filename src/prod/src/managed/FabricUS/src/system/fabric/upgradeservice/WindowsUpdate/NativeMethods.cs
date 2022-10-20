// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    using System.Collections.Specialized;
    using System.Collections;
    using System.Runtime.InteropServices;
    using System;

    /// <summary>      
    /// Native methods for the Windows Update Agent API.      
    /// </summary>      
    internal class NativeMethods
    {
        #region OperationResultCode enum

        internal enum OperationResultCode
        {
            NotStarted,
            InProgress,
            Succeeded,
            SucceededWithErrors,
            Failed,
            Aborted
        }

        #endregion

        #region ServerSelection enum
        internal enum ServerSelection : int
        {
            ssDefault = 0,
            ssManagedServer = 1,
            ssWindowsUpdate = 2,
            ssOthers = 3
        }

        #endregion

        #region Download
        [ComImport]
        [Guid("77254866-9F5B-4C8E-B9E2-C77A8530D64B")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IDownloadCompletedCallback
        {
            void Invoke(IDownloadJob downloadJob, IDownloadCompletedCallbackArgs callbackArgs);
        }

        [ComImport]
        [Guid("FA565B23-498C-47A0-979D-E7D5B1813360")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IDownloadCompletedCallbackArgs
        {
        }

        [ComImport]
        [Guid("C574DE85-7358-43F6-AAE8-8697E62D8BA7")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IDownloadJob
        {
            object AsyncState { get; }
            bool IsCompleted { get; }
            IUpdateCollection Updates { get; }
            void CleanUp();
            IDownloadProgress GetProgress();
            void RequestAbort();
        }

        [ComImport]
        [Guid("D31A5BAC-F719-4178-9DBB-5E2CB47FD18A")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface IDownloadProgress
        {
            decimal CurrentUpdateBytesDownloaded { get; }
            decimal CurrentUpdateBytesToDownload { get; }
            int CurrentUpdateIndex { get; }
            int PercentComplete { get; }
            decimal TotalBytesDownloaded { get; }
            decimal TotalBytesToDownload { get; }
            IUpdateDownloadResult GetUpdateResult(int updateIndex);
            int /*DownloadPhase*/ CurrentUpdateDownloadPhase { get; }
            int CurrentUpdatePercentComplete { get; }
        }

        [ComImport]
        [Guid("8C3F1CDD-6173-4591-AEBD-A56A53CA77C1")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IDownloadProgressChangedCallback
        {
            void Invoke(
                IDownloadJob downloadJob,
                IDownloadProgressChangedCallbackArgs callbackArgs);
        }

        [ComImport]
        [Guid("324FF2C6-4981-4B04-9412-57481745AB24")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface IDownloadProgressChangedCallbackArgs
        {
            IDownloadProgress Progress { get; }
        }

        [ComImport]
        [Guid("DAA4FDD0-4727-4DBE-A1E7-745DCA317144")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface IDownloadResult
        {
            int HResult { get; }
            OperationResultCode ResultCode { get; }
            IUpdateDownloadResult GetUpdateResult(int updateIndex);
        }

        [ComImport]
        [Guid("68F1C6F9-7ECC-4666-A464-247FE12496C3")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface IUpdateDownloader
        {
            string ClientApplicationID { get; set; }
            bool IsForced { get; set; }
            int /*DownloadPriority*/ Priority { get; set; }
            IUpdateCollection Updates { get; set; }
            IDownloadJob BeginDownload(object onProgressChanged, object onCompleted, object state);
            IDownloadResult Download();
            IDownloadResult EndDownload(IDownloadJob value);
        }

        [ComImport]
        [Guid("BF99AF76-B575-42AD-8AA4-33CBB5477AF1")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface IUpdateDownloadResult
        {
            int HResult { get; }
            OperationResultCode ResultCode { get; }
        }

        #endregion
        
        #region Search
        [ComImport]
        [Guid("D40CFF62-E08C-4498-941A-01E25F0FD33C")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface ISearchResult
        {
            OperationResultCode ResultCode { get; }
            object /*ICategoryCollection*/ RootCategories { get; }
            IUpdateCollection Updates { get; }
            object /*IUpdateExceptionCollection*/ Warnings { get; }
        }

        [ComImport]
        [Guid("88AEE058-D4B0-4725-A2F1-814A67AE964C")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface ISearchCompletedCallback
        {
            void Invoke(ISearchJob searchJob, ISearchCompletedCallbackArgs callbackArgs);
        }

        [Guid("7366EA16-7A1A-4EA2-B042-973D3E9CD99B")]
        [ComImport]
        public interface ISearchJob
        {
            dynamic AsyncState { get; }
            bool IsCompleted { get; }
            void CleanUp();
            void RequestAbort();
        }

        [ComImport]
        [Guid("A700A634-2850-4C47-938A-9E4B6E5AF9A6")]
        public interface ISearchCompletedCallbackArgs
        {
        }

        [ComImport]
        [Guid("8F45ABF1-F9AE-4B95-A933-F0F66E5056EA")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        public interface IUpdateSearcher
        {
            bool CanAutomaticallyUpgradeService { get; set; }
            string ClientApplicationID { get; set; }
            bool IncludePotentiallySupersededUpdates { get; set; }
            bool Online { get; set; }
            ServerSelection ServerSelection { get; set; }
            string ServiceID { get; set; }
            ISearchJob BeginSearch(string criteria, object onCompleted, object state);
            ISearchResult EndSearch(ISearchJob searchJob);
        }

        [ComImport]
        [Guid("4CBDCB2D-1589-4BEB-BD1C-3E582FF0ADD0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface IUpdateSearcher2
        {
            bool CanAutomaticallyUpgradeService { get; set; }
            string ClientApplicationID { get; set; }
            bool IncludePotentiallySupersededUpdates { get; set; }
            ServerSelection ServerSelection { get; set; }
            object /*ISearchJob*/ BeginSearch(string criteria, object onCompleted, object state);
            ISearchResult EndSearch(object /*ISearchJob*/ searchJob);
            string EscapeString(string unescaped);
            object /*IUpdateHistoryEntryCollection*/ QueryHistory(int startIndex, int Count);
            ISearchResult Search(string criteria);
            bool Online { get; set; }
            int GetTotalHistoryCount();
            string ServiceID { get; set; }
            bool IgnoreDownloadPriority { get; set; }
        }

        #endregion

        #region Session
        [ComImport]
        [Guid("918EFD1E-B5D8-4C90-8540-AEB9BDC56F9D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        public interface IUpdateSession3
        {
            string ClientApplicationID { get; set; }
            bool ReadOnly { get; }
            uint UserLocale { get; set; }
            object /* WebProxy */ WebProxy { get; set; }

            IUpdateDownloader CreateUpdateDownloader();
            IUpdateSearcher CreateUpdateSearcher();
        }
        
        #endregion

        #region UpdateSource enum

        [ComVisible(true)]
        public enum UpdateSource
        {
            MicrosoftUpdate = 0,
            Other = 1
        }

        #endregion

        #region MsrcSeverity enum

        [ComVisible(true)]
        public enum MsrcSeverity
        {
            Unspecified = 0,
            Low = 1,
            Moderate = 2,
            Important = 3,
            Critical = 4,
        }

        #endregion

        #region UpdateType enum

        [ComVisible(true)]
        public enum UpdateType
        {
            Software = 1,
            Driver = 2,
            SoftwareApplication = 5,
            DriverSet = 6
        }

        #endregion

        #region UpdateTypes enum

        [ComVisible(true)]
        [Flags]
        public enum UpdateTypes
        {
            Driver = 0x0000001,
            SoftwareUpdate = 0x00000002,
            SoftwareApplication = 0x00000004,
            DriverSet = 0x00000008,
            All = unchecked((int)0xFFFFFFFF)
        }

        #endregion

        #region UpdateState enum

        [ComVisible(true)]
        public enum UpdateState
        {
            LicenseAgreementNotReady = 1,
            InstallationImpossible = 2,
            NotNeeded = 3,
            NotReady = 4,
            Ready = 5,
            Canceled = 6,
            Failed = 7,
            LicenseAgreementFailed = 8,
        }

        #endregion

        #region PublicationState enum

        [ComVisible(true)]
        public enum PublicationState
        {
            Published = 0,
            Expired = 1
        }

        #endregion

        #region IUpdate
        [ComImport]
        [Guid("4CB43D7F-7EEE-4906-8698-60DA1C38F2FE")]
        internal class UpdateSession
        {
        }

        [Guid("46297823-9940-4C09-AED9-CD3EA6D05968")]
        public interface IUpdateIdentity
        {
            int RevisionNumber { get; }
            string UpdateID { get; }
        }

        [ComImport]
        [Guid("6A92B07A-D821-4682-B423-5C805022CC4D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        public interface IUpdate
        {
            #region Properties

            IUpdateIdentity Identity { get; }

            string Title { get; }

            bool EulaAccepted { get; }

            bool IsDownloaded { get; }

            string Description { get; }

            string LegacyName { get; }

            MsrcSeverity MsrcSeverity { get; }

            StringCollection KnowledgebaseArticles { get; }

            StringCollection SecurityBulletins { get; }

            StringCollection AdditionalInformationUrls { get; }

            string UpdateClassificationTitle { get; }

            StringCollection CompanyTitles { get; }

            StringCollection ProductTitles { get; }

            StringCollection ProductFamilyTitles { get; }

            DateTime CreationDate { get; }

            DateTime ArrivalDate { get; }

            UpdateType UpdateType { get; }

            PublicationState PublicationState { get; }

            IInstallationBehavior InstallationBehavior { get; }

            IInstallationBehavior UninstallationBehavior { get; }

            bool IsApproved { get; }

            bool IsDeclined { get; }

            bool HasStaleUpdateApprovals { get; }

            bool IsLatestRevision { get; }

            bool HasEarlierRevision { get; }

            UpdateState State { get; }

            bool HasLicenseAgreement { get; }

            bool RequiresLicenseAgreementAcceptance { get; }

            bool HasSupersededUpdates { get; }

            bool IsSuperseded { get; }

            bool IsWsusInfrastructureUpdate { get; }

            bool IsEditable { get; }

            UpdateSource UpdateSource { get; }

            #endregion

            #region Methods

            void Refresh();

            void CopyFromCache(string path, bool toExtractCabFiles);

            void AcceptLicenseAgreement();

            #endregion
        }

        #endregion

        #region Update Collection
        [ComImport]
        [Guid("07F7438C-7709-4CA5-B518-91279288134E")]
        [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
        internal interface IUpdateCollection /*: IEnumerable*/
        {
            IUpdate this[int index] { get; set; }
            IEnumerator GetEnumerator();
            int Count { get; }
            bool ReadOnly { get; }
            int Add(IUpdate value);
            void Clear();
            UpdateCollection Copy();
            void Insert(int index, IUpdate value);
            void RemoveAt(int index);
        }

        [ComImport]
        [Guid("13639463-00DB-4646-803D-528026140D88")]
        public class UpdateCollection
        {
        }

        #endregion

        #region InstallationBehavior class

        [Guid("D9A59339-E245-4DBD-9686-4D5763E39624")]
        [ComVisible(true)]
        public interface IInstallationBehavior
        {
            bool CanRequestUserInput { get; }
            InstallationImpact Impact { get; }
            InstallationRebootBehavior RebootBehavior { get; }
            bool RequiresNetworkConnectivity { get; }
        }

        public enum InstallationRebootBehavior
        {
            irbNeverReboots = 0,
            irbAlwaysRequiresReboot = 1,
            irbCanRequestReboot = 2,
        }

        #endregion

        #region InstallationImpact enum
        [ComVisible(true)]
        public enum InstallationImpact
        {
            Normal = 0,
            Minor = 1,
            RequiresExclusiveHandling = 2
        }
        #endregion
    }
}