#include <app/app.h>

// We're going to halt development of the app for
// now and focus on the system that will behave similar
// to an ECS, but not exactly an ECS. Fun little experiments
// to come soon!

// TODO: Instead of making an application class that owns a telemetry publisher
// maybe we could make a TelemetryLayer : ILayer, that way the application started from
// whatever game is using the framework wouldn't need to know about a telemetry publisher
// or care about updating it. LayerStack would update it on updateAll().
