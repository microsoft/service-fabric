// ------------------------------------------------------------
// DO NOT OPEN SOURCE
// ------------------------------------------------------------

namespace FabricDCA
{
    public static class AppInsightsTelemetryKey
    {
#if !DEBUG
        public const string AppInsightsInstrumentationKey = "AIF-58ef8eab-a250-4b11-aea8-36435e5be1a7";
#else
        // different instrumentation key to separate development from production data
        public const string AppInsightsInstrumentationKey = "AIF-c12b7a37-76f8-4221-9282-0dd64a31d14b";
#endif
        public const string AppInsightsEndPointAddress = "https://vortex.data.microsoft.com/collect/v1";
    }
}