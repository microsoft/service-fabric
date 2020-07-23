// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.LockManager
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Fabric.Strings;
    using System.Globalization;


    /// <summary>
    /// Lock manager implementation.
    /// </summary>
    sealed class LockManager : Disposable
    {
        #region Instance Members

        /// <summary>
        /// Lock compatibility matrix.
        /// </summary>
        static Dictionary<LockMode, Dictionary<LockMode, LockCompatibility>> lockCompatibility =
            new Dictionary<LockMode, Dictionary<LockMode, LockCompatibility>>();

        /// <summary>
        /// Max compatible lock matrix.
        /// </summary>
        static Dictionary<LockMode, Dictionary<LockMode, LockMode>> lockConversion = new Dictionary<LockMode, Dictionary<LockMode, LockMode>>();

        /// <summary>
        /// Locks.
        /// </summary>
        LockHashtable lockHashTable = new LockHashtable();

        /// <summary>
        /// Keeps the Open/Closed status of this lock manager.
        /// </summary>
        bool status;

        /// <summary>
        /// The lock manager will wait for all locks to be released during closing. 
        /// </summary>
        TaskCompletionSource<bool> drainAllLocks;

        /// <summary>
        // Owner creating this lock manager.
        /// </summary>
        Uri owner;

        #endregion

        /// <summary>
        /// Initialize lock conversion and lock compatibility tables.
        /// </summary>
        public LockManager()
        {
            //
            // Initialize compatibility and conversion matrices (price paid only once per process).
            //
            lock (LockManager.lockConversion)
            {
                if (0 == LockManager.lockConversion.Count)
                {
                    //
                    // Granted is the first key, requested is the second key, value is the max.
                    //
                    LockManager.lockConversion.Add(LockMode.Free, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.SchemaStability, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.SchemaModification, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.Shared, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.Update, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.Exclusive, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.IntentShared, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.IntentUpdate, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.IntentExclusive, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.SharedIntentUpdate, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.SharedIntentExclusive, new Dictionary<LockMode, LockMode>());
                    LockManager.lockConversion.Add(LockMode.UpdateIntentExclusive, new Dictionary<LockMode, LockMode>());

                    LockManager.lockConversion[LockMode.Free].Add(LockMode.Free, LockMode.Free);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.SchemaStability, LockMode.SchemaStability);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.Shared, LockMode.Shared);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.Update, LockMode.Update);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.IntentShared, LockMode.IntentShared);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.IntentUpdate, LockMode.IntentUpdate);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.Free].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);
                    
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.Free, LockMode.SchemaStability);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.SchemaStability, LockMode.SchemaStability);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.Shared, LockMode.Shared);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.Update, LockMode.Update);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.IntentShared, LockMode.IntentShared);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.IntentUpdate, LockMode.IntentUpdate);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SchemaStability].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.Free, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.SchemaStability, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.Shared, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.Update, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.Exclusive, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.IntentShared, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.IntentUpdate, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.IntentExclusive, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.SharedIntentUpdate, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.SharedIntentExclusive, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SchemaModification].Add(LockMode.UpdateIntentExclusive, LockMode.SchemaModification);

                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.Free, LockMode.Shared);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.SchemaStability, LockMode.Shared);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.Shared, LockMode.Shared);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.Update, LockMode.Update);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.IntentShared, LockMode.Shared);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.IntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.IntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.Shared].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.Update].Add(LockMode.Free, LockMode.Update);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.SchemaStability, LockMode.Update);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.Shared, LockMode.Update);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.Update, LockMode.Update);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.IntentShared, LockMode.Update);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.IntentUpdate, LockMode.Update);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.IntentExclusive, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.SharedIntentUpdate, LockMode.Update);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.SharedIntentExclusive, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.Update].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.Free, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.SchemaStability, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.Shared, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.Update, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.IntentShared, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.IntentUpdate, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.IntentExclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.SharedIntentUpdate, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.SharedIntentExclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.Exclusive].Add(LockMode.UpdateIntentExclusive, LockMode.Exclusive);

                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.Free, LockMode.IntentShared);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.SchemaStability, LockMode.IntentShared);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.Shared, LockMode.Shared);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.Update, LockMode.Update);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.IntentShared, LockMode.IntentShared);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.IntentUpdate, LockMode.IntentUpdate);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.IntentShared].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.Free, LockMode.IntentUpdate);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.SchemaStability, LockMode.IntentUpdate);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.Shared, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.Update, LockMode.Update);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.IntentShared, LockMode.IntentUpdate);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.IntentUpdate, LockMode.IntentUpdate);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.IntentUpdate].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.Free, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.SchemaStability, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.Shared, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.Update, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.IntentShared, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.IntentUpdate, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.IntentExclusive, LockMode.IntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.IntentExclusive].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.Free, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.SchemaStability, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.Shared, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.Update, LockMode.Update);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.IntentShared, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.IntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.IntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentUpdate);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentUpdate].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.Free, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.SchemaStability, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.Shared, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.Update, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.IntentShared, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.IntentUpdate, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.IntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.SharedIntentUpdate, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.SharedIntentExclusive, LockMode.SharedIntentExclusive);
                    LockManager.lockConversion[LockMode.SharedIntentExclusive].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.Free, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.SchemaStability, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.SchemaModification, LockMode.SchemaModification);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.Shared, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.Update, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.Exclusive, LockMode.Exclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.IntentShared, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.IntentUpdate, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.IntentExclusive, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.SharedIntentUpdate, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.SharedIntentExclusive, LockMode.UpdateIntentExclusive);
                    LockManager.lockConversion[LockMode.UpdateIntentExclusive].Add(LockMode.UpdateIntentExclusive, LockMode.UpdateIntentExclusive);

                    //
                    // Granted is the first key, requested is the second key, value is the result of compatibility.
                    //
                    LockManager.lockCompatibility.Add(LockMode.Free, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.SchemaStability, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.SchemaModification, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.Shared, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.Update, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.Exclusive, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.IntentShared, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.IntentUpdate, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.IntentExclusive, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.SharedIntentUpdate, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.SharedIntentExclusive, new Dictionary<LockMode, LockCompatibility>());
                    LockManager.lockCompatibility.Add(LockMode.UpdateIntentExclusive, new Dictionary<LockMode, LockCompatibility>());
                    
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.SchemaModification, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.Shared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.Update, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.Exclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.SharedIntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.SharedIntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Free].Add(LockMode.UpdateIntentExclusive, LockCompatibility.NoConflict);

                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.Shared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.Update, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.Exclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.SharedIntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.SharedIntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaStability].Add(LockMode.UpdateIntentExclusive, LockCompatibility.NoConflict);
                    
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.SchemaStability, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.Shared, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.IntentShared, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.IntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.SharedIntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SchemaModification].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);

                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.Shared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.Update, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.SharedIntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Shared].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);
                    
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.Shared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.IntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.SharedIntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Update].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);

                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.Shared, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.IntentShared, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.IntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.SharedIntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.Exclusive].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);

                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.Shared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.Update, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.SharedIntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.SharedIntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentShared].Add(LockMode.UpdateIntentExclusive, LockCompatibility.NoConflict);

                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.Shared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.SharedIntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.SharedIntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentUpdate].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);

                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Shared, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.IntentExclusive, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.SharedIntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.IntentExclusive].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);

                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.Shared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.SharedIntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentUpdate].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);

                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.Shared, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.IntentUpdate, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.SharedIntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.SharedIntentExclusive].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);

                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.Free, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.SchemaStability, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.SchemaModification, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.Shared, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.Update, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.Exclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.IntentShared, LockCompatibility.NoConflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.IntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.IntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.SharedIntentUpdate, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.SharedIntentExclusive, LockCompatibility.Conflict);
                    LockManager.lockCompatibility[LockMode.UpdateIntentExclusive].Add(LockMode.UpdateIntentExclusive, LockCompatibility.Conflict);
                }
            }
        }

        /// <summary>
        /// Initializes all resources required for the lock manager.
        /// </summary>
        /// <returns></returns>
        public Task OpenAsync(Uri owner, CancellationToken cancellationToken)
        {
            //
            // Check arguments.
            //
            if (null == owner)
            {
                throw new ArgumentNullException("owner");
            }

            TaskCompletionSource<bool> result = new TaskCompletionSource<bool>();
            result.SetResult(true);
            //
            // Remember the owenr for tracing purposes.
            //
            this.owner = owner;

            AppTrace.TraceSource.WriteNoise("LockManager.Open", "{0}", this.owner);

            //
            // Create drain task.
            //
            this.drainAllLocks = new TaskCompletionSource<bool>();
            //
            // Set the status of this lock manager to be open for business.
            //
            this.status = true;

            return result.Task;
        }

        /// <summary>
        /// Closes the lock manager and waits for all locks to be released. Times out all pending lock requests.
        /// </summary>
        /// <returns></returns>
        public Task CloseAsync(CancellationToken cancellationToken)
        {
            return this.CloseAsync(true, cancellationToken);
        }

        /// <summary>
        /// The lock manager is closing immediately.
        /// </summary>
        public void Abort()
        {
            this.CloseAsync(false, CancellationToken.None).Wait();
        }

        /// <summary>
        /// Closes the lock manager and waits for all locks to be released. Times out all pending lock requests.
        /// </summary>
        /// <returns></returns>
        async Task CloseAsync(bool drain, CancellationToken cancellationToken)
        {
            AppTrace.TraceSource.WriteNoise("LockManager.Close", "{0} - drain {1}", this.owner, drain);

            int countGranted = 0;
            //
            // Acquire first level lock.
            //
            this.lockHashTable.lockHashTableLock.EnterWriteLock();
            if (this.status)
            {
                //
                // Set the status to closed. New lock request will be timed-out immediately.
                //
                this.status = false;
                //
                // Enumerate all entries.
                //
                using (Dictionary<string, LockHashValue>.Enumerator enumerate = this.lockHashTable.lockEntries.GetEnumerator())
                {
                    while (enumerate.MoveNext())
                    {
                        //
                        // Lock each entry.
                        //
                        enumerate.Current.Value.lockHashValueLock.EnterWriteLock();
                        //
                        // Time out all waiters.
                        //
                        foreach (var x in enumerate.Current.Value.lockResourceControlBlock.waitingQueue)
                        {
                            //
                            // Internal state checks.
                            //
                            Debug.Assert(LockStatus.Pending == x.lockStatus);
                            //
                            // Stop the expiration timer for this waiter.
                            //
                            if (x.StopExpire())
                            {
                                x.Dispose();
                            }
                            //
                            // Set the timed out lock status.
                            //
                            x.lockStatus = LockStatus.Timeout;

                            AppTrace.TraceSource.WriteNoise(
                                "LockManager.Close",
                                "Lock request ({0}, {1}, {2}, {3}, {4})",
                                this.owner,
                                x.lockOwner,
                                x.lockResourceName,
                                x.lockMode,
                                x.lockStatus);

                            //
                            // Complete the pending waiter task.
                            //
                            x.waiter.SetResult(x);
                        }
                        //
                        // Clear waiting queue.
                        //
                        enumerate.Current.Value.lockResourceControlBlock.waitingQueue.Clear();
                        //
                        // Keep track of granted lock count.
                        //
                        countGranted += enumerate.Current.Value.lockResourceControlBlock.grantedList.Count;
                        //
                        // Unlock each entry.
                        //
                        enumerate.Current.Value.lockHashValueLock.ExitWriteLock();
                    }
                }
            }
            //
            // Release first level lock.
            //
            this.lockHashTable.lockHashTableLock.ExitWriteLock();
            //
            // If there are granted locks, then wait from them be unlocked.
            //
            if (0 != countGranted && drain)
            {
                AppTrace.TraceSource.WriteNoise("LockManager.Close", "{0} start drain", this.owner);
                await this.drainAllLocks.Task;
                AppTrace.TraceSource.WriteNoise("LockManager.Close", "{0} drain ended", this.owner);
            }
        }

        /// <summary>
        /// Starts locking a given lock resource in a given mode.
        /// </summary>
        /// <param name="owner">The lock owner for this request.</param>
        /// <param name="resourceName">Resource name to be locked.</param>
        /// <param name="mode">Lock mode required by this request.</param>
        /// <param name="timeout">Specifies the timeout until this request is kept alive.</param>
        /// <exception cref="System.OutOfMemoryException">Operation could not be completed due to insufficient resources.</exception>
        /// <exception cref="System.TimeoutException">A specified timeout has expired.</exception>
        /// <exception cref="System.ArgumentException">The specified timeout is invalid.</exception>
        /// <exception cref="System.ArgumentNullException">Owner or resource name is null.</exception>
        /// <returns></returns>
        public Task<ILock> AcquireLockAsync(Uri owner, string resourceName, LockMode mode, int timeout)
        {
            //
            // Check arguments.
            //
            if (0 > timeout && timeout != System.Threading.Timeout.Infinite)
            {
                throw new ArgumentException(StringResources.Error_InvalidTimeOut, "timeout");
            }
            if (LockMode.Free == mode)
            {
                throw new ArgumentException(StringResources.Error_InvalidLockMode, "mode");
            }
            if (null == owner)
            {
                throw new ArgumentNullException("owner");
            }
            if (string.IsNullOrEmpty(resourceName) || string.IsNullOrWhiteSpace(resourceName))
            {
                throw new ArgumentNullException("resourceName");
            }
            LockHashValue hashValue = null;
            TaskCompletionSource<ILock> completion = new TaskCompletionSource<ILock>();
            bool isGranted = false;
            bool isPending = false;
            bool duplicate = false;
            bool isUpgrade = false;
            //
            // Acquire first level lock.
            //
            this.lockHashTable.lockHashTableLock.EnterWriteLock();
            //
            // Check to see if the lock manager can accept requests.
            //
            if (!this.status)
            {
                AppTrace.TraceSource.WriteError(
                    "LockManager.AcquireLock", 
                    "Lock request ({0}, {1}, {2}, {3}, {4})", 
                    this.owner, 
                    owner, 
                    resourceName, 
                    mode, 
                    LockStatus.Timeout);

                //
                // Release first level lock.
                //
                this.lockHashTable.lockHashTableLock.ExitWriteLock();
                //
                // Timeout lock request immediately.
                //
                completion.SetResult(new LockControlBlock(null, owner, resourceName, mode, timeout, LockStatus.Timeout, false));
                return completion.Task;
            }
            //
            // Find the lock resource entry, if it exists.
            //
            if (this.lockHashTable.lockEntries.ContainsKey(resourceName))
            {
                //
                // Retrieve hash entry under hash table lock.
                //
                hashValue = this.lockHashTable.lockEntries[resourceName];
                //
                // Acquire second level lock.
                //
                hashValue.lockHashValueLock.EnterWriteLock();
                //
                // Release first level lock.
                //
                this.lockHashTable.lockHashTableLock.ExitWriteLock();
                try
                {
                    //
                    // Attempt to find the lock owner in the granted list.
                    //
                    LockControlBlock lockControlBlock = hashValue.lockResourceControlBlock.LocateLockClient(owner, false);
                    if (null == lockControlBlock)
                    {
                        //
                        // Lock owner not found in the granted list. 
                        //
                        if (0 != hashValue.lockResourceControlBlock.waitingQueue.Count)
                        {
                            //
                            // There are already waiters for this lock. The request therefore cannot be granted in fairness.
                            //
                            isPending = true;
                        }
                        else
                        {
                            //
                            // There are no waiters for the this lock. Grant only if the lock mode is compatible with the existent granted mode.
                            //
                            if (LockManager.IsCompatible(mode, hashValue.lockResourceControlBlock.lockModeGranted))
                            {
                                isGranted = true;
                            }
                            else
                            {
                                isPending = true;
                            }
                        }
                    }
                    else
                    {
                        //
                        // Lock owner found in the granted list.
                        //
                        if (mode == lockControlBlock.lockMode)
                        {
                            //
                            // This is a duplicate request from the same lock owner with the same lock mode.
                            // Grant it immediately, since it is already granted.
                            //
                            isGranted = true;
                            duplicate = true;
                        }
                        else
                        {
                            //
                            // The lock owner is requesting a new lock mode (upgrade or downgrade).
                            // There are two cases for which the request will be granted.
                            // 1. The lock mode requested is compatible with the existent granted mode.
                            // 2. There is a single lock owner in the granted list.
                            //
                            if (LockManager.IsCompatible(mode, hashValue.lockResourceControlBlock.lockModeGranted) || 
                                hashValue.lockResourceControlBlock.IsSingleLockOwnerGranted(owner))
                            {
                                //
                                // Fairness might be violated in this case, in case existent waiters exist.
                                // The reason for violating fairness is that the lock is already granted to this lock owner.
                                //
                                isGranted = true;
                            }
                            else
                            {
                                isPending = true;
                                isUpgrade = true;
                            }
                        }
                    }
                    //
                    // Internal state checks.
                    //
                    Debug.Assert((isGranted || isPending) && !(isGranted && isPending));
                    if (isGranted)
                    {
                        //
                        // Check to see if the lock request is a duplicate request.
                        //
                        if (!duplicate)
                        {
                            //
                            // The lock request is added to the granted list and the granted mode is re-computed.
                            //
                            lockControlBlock = new LockControlBlock(this, owner, resourceName, mode, timeout, LockStatus.Granted, false);
                            lockControlBlock.grantTime = DateTime.UtcNow.Ticks;
                            //
                            // Grant the max lock mode.
                            //
                            hashValue.lockResourceControlBlock.grantedList.Add(lockControlBlock);
                            //
                            // Set the lock resource granted lock status.
                            //
                            hashValue.lockResourceControlBlock.lockModeGranted = LockManager.ConvertToMaxLockMode(
                                mode,
                                hashValue.lockResourceControlBlock.lockModeGranted);
                        }
                        else
                        {
                            //
                            // Return existent lock control block.
                            //
                            Debug.Assert(null != lockControlBlock);
                            lockControlBlock.lockCount++;
                        }

                        AppTrace.TraceSource.WriteNoise(
                            "LockManager.AcquireLock", 
                            "Lock request ({0}, {1}, {2}, {3}, {4})", 
                            this.owner, 
                            lockControlBlock.lockOwner, 
                            lockControlBlock.lockResourceName, 
                            lockControlBlock.lockMode, 
                            lockControlBlock.lockStatus);

                        //
                        // Return immediately.
                        //
                        completion.SetResult(lockControlBlock);
                        return completion.Task;
                    }
                    else
                    {
                        //
                        // Check to see if this is an instant lock.
                        //
                        if (0 == timeout)
                        {
                            AppTrace.TraceSource.WriteNoise(
                                "LockManager.AcquireLock", 
                                "Lock request ({0}, {1}, {2}, {3}, {4})", 
                                this.owner, 
                                lockControlBlock.lockOwner, 
                                lockControlBlock.lockResourceName, 
                                lockControlBlock.lockMode, 
                                lockControlBlock.lockStatus);

                            //
                            // Timeout lock request immediately.
                            //
                            completion.SetResult(new LockControlBlock(null, owner, resourceName, mode, timeout, LockStatus.Timeout, false));
                            return completion.Task;
                        }
                        //
                        // The lock request is added to the waiting list.
                        //
                        lockControlBlock = new LockControlBlock(this, owner, resourceName, mode, timeout, LockStatus.Pending, isUpgrade);
                        if (!isUpgrade)
                        {
                            //
                            // Add the new lock control block to the waiting queue.
                            //
                            hashValue.lockResourceControlBlock.waitingQueue.Add(lockControlBlock);
                        }
                        else
                        {
                            //
                            // Add the new lock control block before any lock control block that is not upgraded, 
                            // but after the last upgraded lock control block.
                            //
                            int index = 0;
                            for (; index < hashValue.lockResourceControlBlock.waitingQueue.Count; index++)
                            {
                                if (!hashValue.lockResourceControlBlock.waitingQueue[index].IsUpgraded)
                                {
                                    break;
                                }
                            }
                            if (index == hashValue.lockResourceControlBlock.waitingQueue.Count)
                            {
                                hashValue.lockResourceControlBlock.waitingQueue.Add(lockControlBlock);
                            }
                            else
                            {
                                hashValue.lockResourceControlBlock.waitingQueue.Insert(index, lockControlBlock);
                            }
                        }
                        //
                        // Set the waiting completion task source on the lock control block.
                        //
                        lockControlBlock.waiter = completion;
                        //
                        // Start the expiration for this lock control block.
                        //
                        lockControlBlock.StartExpire();

                        AppTrace.TraceSource.WriteNoise(
                            "LockManager.AcquireLock", 
                            "Lock request ({0}, {1}, {2}, {3}, {4})", 
                            this.owner, 
                            lockControlBlock.lockOwner, 
                            lockControlBlock.lockResourceName, 
                            lockControlBlock.lockMode, 
                            lockControlBlock.lockStatus);
                        
                        //
                        // Done with this request. The task is pending.
                        //
                        return completion.Task;
                    }
                }
                finally
                {
                    //
                    // Release second level lock.
                    //
                    hashValue.lockHashValueLock.ExitWriteLock();
                }
            }
            else
            {
                try
                {
                    //
                    // New lock resource being created.
                    //
                    hashValue = new LockHashValue();
                    //
                    // Store lock resource name with its lock control block.
                    //
                    this.lockHashTable.lockEntries.Add(resourceName, hashValue);
                    //
                    // Create new lock resource control block.
                    //
                    hashValue.lockResourceControlBlock = new LockResourceControlBlock();
                    //
                    // Create new lock control block.
                    //
                    LockControlBlock lockControlBlock = new LockControlBlock(this, owner, resourceName, mode, timeout, LockStatus.Granted, false);
                    lockControlBlock.grantTime = DateTime.UtcNow.Ticks;
                    //
                    // Add the new lock control block to the granted list.
                    //
                    hashValue.lockResourceControlBlock.grantedList.Add(lockControlBlock);
                    //
                    // Set the lock resource granted lock status.
                    //
                    hashValue.lockResourceControlBlock.lockModeGranted = LockManager.ConvertToMaxLockMode(mode, LockMode.Free);

                    AppTrace.TraceSource.WriteNoise(
                        "LockManager.AcquireLock", 
                        "Lock request ({0}, {1}, {2}, {3}, {4})", 
                        this.owner, 
                        lockControlBlock.lockOwner, 
                        lockControlBlock.lockResourceName, 
                        lockControlBlock.lockMode, 
                        lockControlBlock.lockStatus);

                    //
                    // Return immediately.
                    //
                    completion.SetResult(lockControlBlock);
                    return completion.Task;
                }
                finally
                {
                    //
                    // Release first level lock.
                    //
                    this.lockHashTable.lockHashTableLock.ExitWriteLock();
                }
            }
        }

        /// <summary>
        /// Recomputes the lock granted mode for a given lock resource. 
        /// It can be called when a lock request has expired or when a granted lock is released.
        /// </summary>
        /// <param name="lockHashValue">Lock hash value containing the lock resource.</param>
        /// <param name="lockResourceName">Lock resource that requires the lock granted recalculation.</param>
        void RecomputeLockGrantees(LockHashValue lockHashValue, string lockResourceName)
        {
            LockControlBlock lockControlBlock = null;
            //
            // Need to find waiters that can be woken up.
            //
            List<LockControlBlock> waitersWokenUpSuccess = new List<LockControlBlock>();
            //
            // Check if there is only one lock owner in the granted list.
            //
            bool isSingleLockOwner = lockHashValue.lockResourceControlBlock.IsSingleLockOwnerGranted(null);
            //
            // Recompute lock granted mode.
            //
            lockHashValue.lockResourceControlBlock.lockModeGranted = LockMode.Free;
            //
            // Go over the existent granted list first.
            //
            for (int index = 0; index < lockHashValue.lockResourceControlBlock.grantedList.Count; index++)
            {
                lockControlBlock = lockHashValue.lockResourceControlBlock.grantedList[index];
                //
                // Internal state checks.
                //
                Debug.Assert(lockControlBlock.lockStatus == LockStatus.Granted);
                Debug.Assert(LockManager.IsCompatible(lockControlBlock.lockMode, lockHashValue.lockResourceControlBlock.lockModeGranted) || isSingleLockOwner);
                //
                // Upgrade the lock mode.
                //
                lockHashValue.lockResourceControlBlock.lockModeGranted = LockManager.ConvertToMaxLockMode(
                    lockControlBlock.lockMode,
                    lockHashValue.lockResourceControlBlock.lockModeGranted);

                AppTrace.TraceSource.WriteNoise(
                    "LockManager.RecomputeLockGrantees",
                    "Granted Lock mode ({0} {1})",
                    lockResourceName,
                    lockHashValue.lockResourceControlBlock.lockModeGranted);
            }
            //
            // Go over remaining waiting queue and determine if any new waiter can be granted the lock.
            //
            for (int index = 0; index < lockHashValue.lockResourceControlBlock.waitingQueue.Count; index++)
            {
                lockControlBlock = lockHashValue.lockResourceControlBlock.waitingQueue[index];
                //
                // Internal state checks.
                //
                Debug.Assert(lockControlBlock.lockStatus == LockStatus.Pending);
                //
                // Check lock compatibility.
                //
                if (LockManager.IsCompatible(lockControlBlock.lockMode, lockHashValue.lockResourceControlBlock.lockModeGranted) ||
                    lockHashValue.lockResourceControlBlock.IsSingleLockOwnerGranted(lockControlBlock.lockOwner))
                {
                    //
                    // Set lock status to success.
                    //
                    lockControlBlock.lockStatus = LockStatus.Granted;
                    lockControlBlock.grantTime = DateTime.UtcNow.Ticks;
                    //
                    // Upgrade the lock mode.
                    //
                    lockHashValue.lockResourceControlBlock.lockModeGranted = LockManager.ConvertToMaxLockMode(
                            lockControlBlock.lockMode,
                            lockHashValue.lockResourceControlBlock.lockModeGranted);
                    //
                    // This waiter has successfully waited for the lock.
                    //
                    waitersWokenUpSuccess.Add(lockControlBlock);
                    lockHashValue.lockResourceControlBlock.grantedList.Add(lockControlBlock);
                    //
                    // Stop the expiration timer for this waiter.
                    //
                    lockControlBlock.StopExpire();

                    AppTrace.TraceSource.WriteNoise(
                        "LockManager.RecomputeLockGrantees", 
                        "Lock request ({0}, {1}, {2}, {3}, {4})", 
                        this.owner, 
                        lockControlBlock.lockOwner, 
                        lockControlBlock.lockResourceName, 
                        lockControlBlock.lockMode, 
                        lockControlBlock.lockStatus);
                }
                else
                {
                    //
                    // Cannot unblock the rest of the waiters since at least one was found 
                    // that has an incompatible lock mode.
                    //
                    break;
                }
            }

            AppTrace.TraceSource.WriteNoise(
                "LockManager.RecomputeLockGrantees", 
                "Lock granted mode ({0}, {1}, {2})", 
                this.owner, 
                lockResourceName, 
                lockHashValue.lockResourceControlBlock.lockModeGranted);

            //
            // Remove all granted waiters from the waiting queue and complete them.
            //
            for (int index = 0; index < waitersWokenUpSuccess.Count; index++)
            {
                lockHashValue.lockResourceControlBlock.waitingQueue.Remove(waitersWokenUpSuccess[index]);
                waitersWokenUpSuccess[index].waiter.SetResult(waitersWokenUpSuccess[index]);
            }
            waitersWokenUpSuccess.Clear();
            //
            // If there are no granted or waiting lock owner, we can clean up this lock resource lazily.
            //
            if (0 == lockHashValue.lockResourceControlBlock.grantedList.Count &&
                0 == lockHashValue.lockResourceControlBlock.waitingQueue.Count)
            {
                Task.Factory.StartNew(() => { this.ClearLock(lockResourceName); });
            }
        }

        /// <summary>
        /// Unlocks a previously granted lock.
        /// </summary>
        /// <param name="acquiredLock">Lock to release.</param>
        /// <returns></returns>
        public UnlockStatus ReleaseLock(ILock acquiredLock)
        {
            //
            // Check arguments.
            //
            if (null == acquiredLock || null == acquiredLock.ResourceName)
            {
                throw new ArgumentNullException("acquiredLock");
            }
            LockHashValue lockHashValue = null;
            LockControlBlock lockControlBlock = acquiredLock as LockControlBlock;
            if (null == lockControlBlock)
            {
                throw new ArgumentException(StringResources.Error_InvalidLockControlBlock, "acquiredLock");
            }
            AppTrace.TraceSource.WriteNoise(
                "LockManager.ReleaseLock",
                "Unlock ({0}, {1})",
                this.owner,
                acquiredLock.ResourceName);
            //
            // Acquire first level lock.
            //
            this.lockHashTable.lockHashTableLock.EnterWriteLock();
            //
            // Find the right lock resource, if it exists.
            //
            if (this.lockHashTable.lockEntries.ContainsKey(lockControlBlock.lockResourceName))
            {
                lockHashValue = this.lockHashTable.lockEntries[lockControlBlock.lockResourceName];
                //
                // Acquire second level lock.
                //
                lockHashValue.lockHashValueLock.EnterWriteLock();
                //
                // Release first level lock.
                //
                this.lockHashTable.lockHashTableLock.ExitWriteLock();
                try
                {
                    if (!lockHashValue.lockResourceControlBlock.grantedList.Contains(lockControlBlock))
                    {
                        AppTrace.TraceSource.WriteNoise(
                            "LockManager.ReleaseLock", 
                            "Unlock ({0}, {1}, {2}, {3})", 
                            this.owner, 
                            acquiredLock.ResourceName, 
                            acquiredLock.Mode,
                            UnlockStatus.NotGranted);

                        //
                        // If no lock owner found, return immediately.
                        //
                        return UnlockStatus.NotGranted;
                    }
                    //
                    // Decrement the lock count for this granted lock.
                    //
                    lockControlBlock.lockCount--;
                    Debug.Assert(0 <= lockControlBlock.lockCount);
                    //
                    // Check if the lock can be removed.
                    //
                    if (0 == lockControlBlock.lockCount)
                    {
                        //
                        // Remove the lock control block for this lock owner.
                        //
                        lockHashValue.lockResourceControlBlock.grantedList.Remove(lockControlBlock);
                        //
                        // This lock control block cannot be reused after this call.
                        //
                        if (lockControlBlock.StopExpire())
                        {
                            lockControlBlock.Dispose();
                        }
                        //
                        // Recompute the new grantees and lock granted mode.
                        //
                        this.RecomputeLockGrantees(lockHashValue, lockControlBlock.lockResourceName);
                    }

                    AppTrace.TraceSource.WriteNoise(
                        "LockManager.ReleaseLock", 
                        "Unlock ({0}, {1}, {2}, {3})", 
                        this.owner, 
                        acquiredLock.ResourceName, 
                        acquiredLock.Mode,
                        UnlockStatus.Success);

                    return UnlockStatus.Success;
                }
                finally
                {
                    //
                    // Release second level lock.
                    //
                    lockHashValue.lockHashValueLock.ExitWriteLock();
                }
            }
            else
            {
                //
                // Release first level lock.
                //
                this.lockHashTable.lockHashTableLock.ExitWriteLock();

                AppTrace.TraceSource.WriteNoise(
                    "LockManager.ReleaseLock", 
                    "Unlock ({0}, {1}, {2}, {3})", 
                    this.owner, 
                    acquiredLock.ResourceName, 
                    acquiredLock.Mode,
                    UnlockStatus.UnknownResource);

                //
                // Return immediately.
                //
                return UnlockStatus.UnknownResource;
            }
        }

        /// <summary>
        /// Returns true, if the lock mode is considered sharable.
        /// </summary>
        /// <param name="mode">Lock mode to inspect.</param>
        /// <returns></returns>
        public bool IsShared(LockMode mode)
        {
            return LockMode.IntentShared == mode || LockMode.Shared == mode || LockMode.SchemaStability == mode;
        }

        /// <summary>
        /// Proceeds with the expiration of a pending lock request.
        /// </summary>
        /// <param name="lockControlBlock">Lock control block representing the pending request.</param>
        /// <returns></returns>
        internal bool ExpireLock(LockControlBlock lockControlBlock)
        {
            //
            // Internal state checks.
            //
            Debug.Assert(null != lockControlBlock);
            Debug.Assert(null != lockControlBlock.lockResourceName);

            LockHashValue lockHashValue = null;
            //
            // Acquire first level lock.
            //
            this.lockHashTable.lockHashTableLock.EnterWriteLock();
            //
            // Find the righ lock resource.
            //
            if (this.lockHashTable.lockEntries.ContainsKey(lockControlBlock.lockResourceName))
            {
                lockHashValue = this.lockHashTable.lockEntries[lockControlBlock.lockResourceName];
                //
                // Acquire second level lock.
                //
                lockHashValue.lockHashValueLock.EnterWriteLock();
                //
                // Release first level lock.
                //
                this.lockHashTable.lockHashTableLock.ExitWriteLock();
                try
                {
                    //
                    // Find the lock control block for this lock owner in the waiting list.
                    //
                    int idx = lockHashValue.lockResourceControlBlock.waitingQueue.FindIndex(x => x == lockControlBlock);
                    if (-1 == idx)
                    {
                        //
                        // There must have been an unlock that granted it in the meantime or it has expired already before.
                        //
                        return false;
                    }
                    //
                    // Internal state checks.
                    //
                    Debug.Assert(lockControlBlock.lockStatus == LockStatus.Pending);
                    //
                    // Remove lock owner from waiting list.
                    //
                    lockHashValue.lockResourceControlBlock.waitingQueue.RemoveAt(idx);
                    //
                    // Clear the lock timer.
                    //
                    lockControlBlock.StopExpire();
                    //
                    // Set the correct lock status.
                    //
                    lockControlBlock.lockStatus = LockStatus.Timeout;
                    lockControlBlock.lockCount = 0;
                    //
                    // Complete the waiter.
                    //
                    lockControlBlock.waiter.SetResult(lockControlBlock);
                    //
                    // Recompute the new grantees and lock granted mode.
                    //
                    if (0 == idx)
                    {
                        AppTrace.TraceSource.WriteNoise(
                            "LockManager.ExpireLock", 
                            "Lock request ({0}, {1}, {2}, {3})", 
                            this.owner, 
                            lockControlBlock.lockResourceName, 
                            lockControlBlock.lockMode,
                            lockControlBlock.lockStatus);

                        this.RecomputeLockGrantees(lockHashValue, lockControlBlock.lockResourceName);
                    }
                    return true;
                }
                finally
                {
                    //
                    // Release second level lock.
                    //
                    lockHashValue.lockHashValueLock.ExitWriteLock();
                }
            }
            else
            {
                //
                // Release first level lock.
                //
                this.lockHashTable.lockHashTableLock.ExitWriteLock();
            }
            return false;
        }

        /// <summary>
        /// Clears an unused lock resource entry.
        /// </summary>
        /// <param name="lockResourceName">Lock resource name to be removed.</param>
        void ClearLock(string lockResourceName)
        {
            LockHashValue lockHashValue = null;
            bool wasCleared = false;
            //
            // Acquire first level lock.
            //
            this.lockHashTable.lockHashTableLock.EnterWriteLock();
            //
            // Find the right lock resource.
            //
            if (this.lockHashTable.lockEntries.ContainsKey(lockResourceName))
            {
                lockHashValue = this.lockHashTable.lockEntries[lockResourceName];
                //
                // Acquire second level lock.
                //
                lockHashValue.lockHashValueLock.EnterWriteLock();
                //
                // If there are no granted or waiting lock owners, we can clean up this lock resource.
                //
                if (0 == lockHashValue.lockResourceControlBlock.grantedList.Count &&
                    0 == lockHashValue.lockResourceControlBlock.waitingQueue.Count)
                {
                    AppTrace.TraceSource.WriteNoise(
                        "LockManager.ClearLock", 
                        "Lock request ({0}, {1})", 
                        this.owner, 
                        lockResourceName);

                    this.lockHashTable.lockEntries.Remove(lockResourceName);
                    wasCleared = true;
                }
                if (!wasCleared && !this.status)
                {
                    foreach (LockControlBlock x in lockHashValue.lockResourceControlBlock.grantedList)
                    {
                        Debug.Assert(LockStatus.Granted == x.lockStatus);
                        AppTrace.TraceSource.WriteNoise(
                            "LockManager.ClearLock", 
                            "Lock request ({0}, {1}, {2}, {3}, {4})", 
                            this.owner, 
                            x.lockOwner, 
                            x.lockResourceName, 
                            x.lockMode, 
                            x.lockCount);
                    }
                }
                //
                // Release second level lock.
                //
                lockHashValue.lockHashValueLock.ExitWriteLock();
                if (wasCleared)
                {
                    //
                    // Clean up hash table value.
                    //
                    lockHashValue.Dispose();
                }
            }
            //
            // If the lock manager closed down and there are no more entries, then signal drain is complete.
            // 
            if (0 == this.lockHashTable.lockEntries.Count && !this.status)
            {
                AppTrace.TraceSource.WriteNoise("LockManager.ClearLock", "{0} locks drained", this.owner);
                this.drainAllLocks.SetResult(true);
            }
            //
            // Release first level lock.
            //
            this.lockHashTable.lockHashTableLock.ExitWriteLock();
        }

        /// <summary>
        /// Verifies the compatibility between and existing lock mode that was granted and
        /// the lock mode requested for the same lock resource.
        /// </summary>
        /// <param name="lockModeRequested"></param>
        /// <param name="lockModeGranted"></param>
        /// <returns></returns>
        static bool IsCompatible(LockMode lockModeRequested, LockMode lockModeGranted)
        {
            return LockCompatibility.NoConflict == LockManager.lockCompatibility[lockModeGranted][lockModeRequested];
        }

        /// <summary>
        /// Returns max lock mode of two lock modes.
        /// </summary>
        /// <param name="lockModeRequested">Existing lock mode.</param>
        /// <param name="lockModeGranted">Additional lock mode.</param>
        /// <returns></returns>
        static LockMode ConvertToMaxLockMode(LockMode lockModeRequested, LockMode lockModeGranted)
        {
            return LockManager.lockConversion[lockModeGranted][lockModeRequested];
        }

        #region IDisposable Overrides

        /// <summary>
        /// 
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    if (null != this.lockHashTable)
                    {
                        this.lockHashTable.Dispose();
                        this.lockHashTable = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }
}