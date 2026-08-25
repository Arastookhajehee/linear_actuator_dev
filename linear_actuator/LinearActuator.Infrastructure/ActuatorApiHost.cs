using LinearActuator.Core;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;

namespace LinearActuator.Infrastructure;

public sealed class ActuatorApiHost : IAsyncDisposable
{
    private readonly ActuatorStateStore stateStore;
    private readonly SerialModuleManager serialModuleManager;
    private WebApplication? app;

    public ActuatorApiHost(ActuatorStateStore stateStore, SerialModuleManager serialModuleManager)
    {
        this.stateStore = stateStore;
        this.serialModuleManager = serialModuleManager;
    }

    public bool IsRunning => app is not null;

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        if (app is not null)
        {
            return;
        }

        WebApplicationBuilder builder = WebApplication.CreateBuilder();
        builder.WebHost.UseUrls($"http://{ActuatorConstants.DefaultApiHost}:{ActuatorConstants.DefaultApiPort}");
        builder.Services.ConfigureHttpJsonOptions(options =>
        {
            options.SerializerOptions.PropertyNameCaseInsensitive = true;
        });
        builder.Services.AddEndpointsApiExplorer();
        builder.Services.AddOpenApiDocument(settings =>
        {
            settings.Title = "Linear Actuator API";
            settings.Version = "v1";
        });

        WebApplication webApp = builder.Build();

        webApp.UseOpenApi();
        webApp.UseSwaggerUi();

        webApp.MapGet("/actuators", () => stateStore.Snapshot())
            .WithName("GetActuators");

        webApp.MapPost("/actuators", async Task<ActuatorState> (ActuatorState state, CancellationToken requestAborted) =>
        {
            stateStore.ReplaceState(state);

            await serialModuleManager.SendTargetsAsync(ActuatorConstants.DefaultModuleId, state, requestAborted);

            return stateStore.Snapshot();
        }).WithName("PostActuators");

        webApp.MapGet("/actuator-bundles", () => stateStore.SnapshotBundle())
            .WithName("GetActuatorBundles");

        webApp.MapPost("/actuator-bundles", async Task<ActuatorStateBundle> (ActuatorStateBundle bundle, CancellationToken requestAborted) =>
        {
            stateStore.ReplaceBundle(bundle);

            foreach (KeyValuePair<string, ActuatorState> module in stateStore.SnapshotBundle().Modules)
            {
                await serialModuleManager.SendTargetsAsync(module.Key, module.Value, requestAborted);
            }

            return stateStore.SnapshotBundle();
        }).WithName("PostActuatorBundles");

        await webApp.StartAsync(cancellationToken);
        app = webApp;
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        if (app is null)
        {
            return;
        }

        await app.StopAsync(cancellationToken);
        await app.DisposeAsync();
        app = null;
    }

    public async ValueTask DisposeAsync()
    {
        await StopAsync();
    }
}
