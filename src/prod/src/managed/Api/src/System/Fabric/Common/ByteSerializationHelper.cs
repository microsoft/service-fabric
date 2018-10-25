// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Fabric.Chaos.DataStructures;

   internal static class ByteSerializationHelper
   {
       private const string TraceType = "ByteSerializationHelper";
       public static byte[] ToBytes(this TimeSpan timeSpan)
       {
           return BitConverter.GetBytes(timeSpan.Ticks);
       }

       public static void FromBytes(this TimeSpan timeSpan, byte[] data)
       {
           long ticks = BitConverter.ToInt64(data, 0);
           timeSpan = new TimeSpan(ticks);
       }

       public static byte[] ToBytes(this DateTime timeStamp)
       {
           return BitConverter.GetBytes(timeStamp.Ticks);
       }

       public static void FromBytes(this DateTime timeStamp, byte[] data)
       {
           long ticks = BitConverter.ToInt64(data, 0);
           timeStamp = new DateTime(ticks);
       }

       public static ChaosEvent GetEventFromBytes(byte[] data)
       {
           ChaosEvent e = null;
           ChaosEventType type = (ChaosEventType)BitConverter.ToInt32(data, 0);

           switch(type)
           {
               case ChaosEventType.Started:
                   {
                       e = new StartedEvent();
                   }

                   break;
               case ChaosEventType.Stopped:
                   {
                       e = new StoppedEvent();
                   }

                   break;
               case ChaosEventType.ExecutingFaults:
                   {
                       e = new ExecutingFaultsEvent();
                   }

                   break;
               case ChaosEventType.ValidationFailed:
                   {
                       e = new ValidationFailedEvent();
                   }

                   break;
               case ChaosEventType.TestError:
                   {
                       e = new TestErrorEvent();
                   }

                   break;
               case ChaosEventType.Waiting:
                   {
                       e = new WaitingEvent();
                   }

                   break;
           }

           if(e != null)
           {
               byte[] eventData = new byte[data.Length - 4];
               Array.Copy(data, 4, eventData, 0, eventData.Length);
               e.FromBytes(eventData);
           }

           return e;
       }

       public static ChaosReportFilter GetReportFilterFromBytes(byte[] data)
       {
           ChaosReportFilter filter = new ChaosReportFilter();
           filter.FromBytes(data);

           return filter;
       }
   }
}