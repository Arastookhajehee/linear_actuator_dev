using LinearActuator.Core;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;

namespace LinearActuator.Infrastructure;

public sealed class ActuatorApiHost : IAsyncDisposable
{
    private readonly ActuatorStateStore stateStore;
    private readonly SerialActuatorConnection serialConnection;
    private WebApplication? app;

    public ActuatorApiHost(ActuatorStateStore stateStore, SerialActuatorConnection serialConnection)
    {
        this.stateStore = stateStore;
        this.serialConnection = serialConnection;
    }

    public bool IsRunning => app is not null;

    public async Task StartAsync(string host, int port, CancellationToken cancellationToken = default)
    {
        if (app is not null)
        {
            return;
        }

        WebApplicationBuilder builder = WebApplication.CreateBuilder();
        builder.WebHost.UseUrls($"http://{host}:{port}");
        builder.Services.ConfigureHttpJsonOptions(options =>
        {
            options.SerializerOptions.PropertyNameCaseInsensitive = true;
        });

        WebApplication webApp = builder.Build();

        webApp.MapGet("/actuators", () => Results.Json(stateStore.Snapshot()));
        webApp.MapPost("/actuators", async (ActuatorState state, CancellationToken requestAborted) =>
        {
            stateStore.ReplaceState(state);

            try
            {
                await serialConnection.SendTargetsAsync(state, requestAborted);
            }
            catch
            {
                // Match the archive behavior: API state remains updated even if serial write fails.
            }

            return Results.Json(stateStore.Snapshot());
        });

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
