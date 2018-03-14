// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseLocalStoreSettings 
        : public Api::IEseLocalStoreSettingsResult
        , public Common::ComponentRoot
        , public Common::TextTraceComponent<Common::TraceTaskCodes::EseStore>
    {
    public:
		EseLocalStoreSettings();
        EseLocalStoreSettings(
            std::wstring const & storeName,
            std::wstring const & dbFolderPath,
            bool enableIncrementalBackup);
        EseLocalStoreSettings(
            std::wstring const & storeName,
            std::wstring const & dbFolderPath,
            int compactionThresholdInMB = 0);
        EseLocalStoreSettings(
            std::wstring const & storeName,
            std::wstring const & dbFolderPath,
            int compactionThresholdInMB,
            int maxCursors);
        EseLocalStoreSettings(
            std::wstring const & storeName,
            std::wstring const & dbFolderPath,
            int compactionThresholdInMB,
            int maxCursors,
            int maxCacheSizeInMB);
        EseLocalStoreSettings(
            std::wstring const & storeName,
            std::wstring const & dbFolderPath,
            int compactionThresholdInMB,
            int maxCursors,
            int maxCacheSizeInMB,
            int minCacheSizeInMB);

    private:

        void InitializeCtor(
            std::wstring const & storeName,
            std::wstring const & dbFolderPath);

    public:

        __declspec(property(get=get_StoreName,put=set_StoreName)) std::wstring const & StoreName;
        __declspec(property(get=get_DbFolderPath)) std::wstring const & DbFolderPath;
        __declspec(property(get=get_LogFileSizeInKB)) int LogFileSizeInKB;
        __declspec(property(get=get_LogBufferSizeInKB)) int LogBufferSizeInKB;
        __declspec(property(get=get_MaxCursors,put=set_MaxCursors)) int MaxCursors;
        __declspec(property(get=get_MaxVerPages,put=set_MaxVerPages)) int MaxVerPages;
        __declspec(property(get=get_MaxAsyncCommitDelay)) Common::TimeSpan MaxAsyncCommitDelay;
		__declspec(property(get=get_IsIncrementalBackupEnabled,put=set_IsIncrementalBackupEnabled)) bool IsIncrementalBackupEnabled;
        __declspec(property(get=get_MaxCacheSizeInMB)) int MaxCacheSizeInMB;
        __declspec(property(get=get_MinCacheSizeInMB)) int MinCacheSizeInMB;
		__declspec(property(get=get_OptimizeTableScans,put=set_OptimizeTableScans)) bool OptimizeTableScans;
        __declspec(property(get=get_MaxDefragFrequency,put=set_MaxDefragFrequency)) Common::TimeSpan MaxDefragFrequency;
        __declspec(property(get=get_DefragThresholdInMB)) int DefragThresholdInMB;
        __declspec(property(get=get_DatabasePageSizeInKB)) int DatabasePageSizeInKB;
        __declspec(property(get=get_CompactionThresholdInMB,put=set_CompactionThresholdInMB)) int CompactionThresholdInMB;
        __declspec(property(get=get_IntrinsicValueThresholdInBytes,put=set_IntrinsicValueThresholdInBytes)) int IntrinsicValueThresholdInBytes;
        __declspec(property(get=get_AssertOnFatalError,put=set_AssertOnFatalError)) bool AssertOnFatalError;
        __declspec(property(get = get_EnableOverwriteOnUpdate, put = set_EnableOverwriteOnUpdate)) bool EnableOverwriteOnUpdate;

        std::wstring const & get_StoreName() const { return storeName_; }
        std::wstring const & get_DbFolderPath() const { return dbFolderPath_; }
        int get_LogFileSizeInKB() const { return logFileSizeInKB_; }
        int get_LogBufferSizeInKB() const { return logBufferSizeInKB_; }
        int get_MaxCursors() const { return maxCursors_; }
        int get_MaxVerPages() const { return maxVerPages_; }
        Common::TimeSpan get_MaxAsyncCommitDelay() const { return maxAsyncCommitDelay_; }
		bool get_IsIncrementalBackupEnabled() const { return enableIncrementalBackup_; }
        int get_MaxCacheSizeInMB() const { return maxCacheSizeInMB_; }
        int get_MinCacheSizeInMB() const { return minCacheSizeInMB_; }
		bool get_OptimizeTableScans() const { return optimizeTableScans_; }
        Common::TimeSpan get_MaxDefragFrequency() const { return maxDefragFrequency_; }
        int get_DefragThresholdInMB() const { return defragThresholdInMB_; }
        int get_DatabasePageSizeInKB() const { return dbPageSizeInKB_; }
        int get_CompactionThresholdInMB() const { return compactionThresholdInMB_; }
        int get_IntrinsicValueThresholdInBytes() const { return intrinsicValueThresholdInBytes_; }
        bool get_AssertOnFatalError() const { return assertOnFatalError_; }
        bool get_EnableOverwriteOnUpdate() const { return enableOverwriteOnUpdate_; }

        void set_StoreName(std::wstring const & value) { storeName_ = value; }
        void set_MaxCursors(int value) { maxCursors_ = value; }
        void set_MaxVerPages(int value) { maxVerPages_ = value; }
		void set_IsIncrementalBackupEnabled(bool value) { enableIncrementalBackup_ = value; }
		void set_OptimizeTableScans(bool value) { optimizeTableScans_ = value; }
		void set_MaxDefragFrequency(Common::TimeSpan value) { maxDefragFrequency_ = value; }
		void set_CompactionThresholdInMB(int value) { compactionThresholdInMB_ = value; }
        void set_IntrinsicValueThresholdInBytes(int value) { intrinsicValueThresholdInBytes_ = value; }
		void set_AssertOnFatalError(bool value) { assertOnFatalError_ = value; }
        void set_EnableOverwriteOnUpdate(bool value) { enableOverwriteOnUpdate_ = value; }

        static HRESULT FromConfig(
            __in IFabricCodePackageActivationContext const * codePackageActivationContext,
            __in LPCWSTR configurationPackageName,
            __in LPCWSTR sectionName,
            __out Api::IEseLocalStoreSettingsResultPtr & internalSettings);

        Common::ErrorCode FromPublicApi(FABRIC_ESE_LOCAL_STORE_SETTINGS const &);
        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_ESE_LOCAL_STORE_SETTINGS &) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:

        // Legacy public APIs pass the store name as a separate parameter from ESE settings.
        // Create a store name here, currently populated from a separate API parameter.
        // In the future, we may support overriding it from config or the public API struct.
        //
        std::wstring storeName_;

        std::wstring dbFolderPath_;
        int logFileSizeInKB_;
        int logBufferSizeInKB_;
        int maxCursors_;
        int maxVerPages_;
        Common::TimeSpan maxAsyncCommitDelay_;

        /// <summary>
        /// Enables incremental backup which involves adjusting settings like 
		/// disabling circular logging etc.
		/// One of the adverse effects of enabling this is that transaction logs can 
		/// fill up disk space between backups. With this disabled, disk space is maintained
        /// by overwriting/replacing older transaction logs if the disk space occupied by
        /// the transaction logs reaches a certain limit.
        /// Use caution while enabling this and ensure that you take periodic 
        /// incremental or full backups as these backup options truncate the transaction
        /// logs thus maintaining disk space. 
        /// Disabling this is not needed if you plan on taking only full backups and not
        /// incremental backups.
        /// </summary>
        bool enableIncrementalBackup_;

        int maxCacheSizeInMB_;
        int minCacheSizeInMB_;

        // Not exposed via public API. Only set internally when PrefetchDatabaseTypes() is called.
        //
        bool optimizeTableScans_;

        Common::TimeSpan maxDefragFrequency_;
        int defragThresholdInMB_;
        int dbPageSizeInKB_;
        int compactionThresholdInMB_;
        int intrinsicValueThresholdInBytes_;

        // Not exposed via public API. Internally set by components.
        bool assertOnFatalError_;

        bool enableOverwriteOnUpdate_;
    };

    typedef std::shared_ptr<EseLocalStoreSettings> EseLocalStoreSettingsSPtr;
}
