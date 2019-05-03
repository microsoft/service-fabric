// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.Enums;

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;

    [AttributeUsage(AttributeTargets.Property |
                    AttributeTargets.Field, AllowMultiple = true)]
    public sealed class RunDaysValidation : ValidationAttribute
    {
        public RunDaysValidation()
        {
            this.RequiresValidationContext = true;
        }

        public override bool RequiresValidationContext { get; }

        protected override ValidationResult IsValid(object value, ValidationContext validationContext)
        {
            TimeBasedBackupSchedule scheduleBasedSchedulePolicy =
                validationContext.ObjectInstance as TimeBasedBackupSchedule;
            if (scheduleBasedSchedulePolicy != null &&
                scheduleBasedSchedulePolicy.ScheduleFrequencyType != BackupScheduleFrequency.Daily &&
                (scheduleBasedSchedulePolicy.RunDays == null ||
                scheduleBasedSchedulePolicy.RunDays.Count == 0))
            {
                return new ValidationResult("The RunDays field is required.");
            }
            return ValidationResult.Success;
        }

    }
}