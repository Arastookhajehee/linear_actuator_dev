using LinearActuator.Core;
using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Newtonsoft.Json;

namespace LinearActuator.Infrastructure;

public sealed class ActuatorApiHost : IAsyncDisposable
{
    private static readonly JsonSerializerSettings JsonSettings = new();

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
        builder.Services.AddEndpointsApiExplorer();
        builder.Services.AddOpenApiDocument(settings =>
        {
            settings.Title = "Linear Actuator API";
            settings.Version = "v1";
        });

        WebApplication webApp = builder.Build();

        webApp.UseOpenApi();
        webApp.UseSwaggerUi();

        webApp.MapGet("/actuators", () => JsonResponse(stateStore.Snapshot()))
            .WithName("GetActuators")
            .Produces<ActuatorState>();

        webApp.MapPost("/actuators", async Task<IResult> (HttpRequest request, CancellationToken requestAborted) =>
        {
            ActuatorState? state = await ReadNewtonsoftJsonAsync<ActuatorState>(request, requestAborted);
            if (state is null)
            {
                return Results.BadRequest("Invalid actuator state JSON.");
            }

            stateStore.ReplaceState(state);

            await serialModuleManager.SendTargetsAsync(ActuatorConstants.DefaultModuleId, state, requestAborted);

            return JsonResponse(stateStore.Snapshot());
        })
            .WithName("PostActuators")
            .Accepts<ActuatorState>("application/json")
            .Produces<ActuatorState>()
            .Produces(StatusCodes.Status400BadRequest);

        webApp.MapGet("/actuator-bundles", () => JsonResponse(stateStore.SnapshotBundle()))
            .WithName("GetActuatorBundles")
            .Produces<ActuatorStateBundle>();

        webApp.MapPost("/actuator-bundles", async Task<IResult> (HttpRequest request, CancellationToken requestAborted) =>
        {
            ActuatorStateBundle? bundle = await ReadNewtonsoftJsonAsync<ActuatorStateBundle>(request, requestAborted);
            if (bundle is null)
            {
                return Results.BadRequest("Invalid actuator bundle JSON.");
            }

            stateStore.ReplaceBundle(bundle);

            foreach (KeyValuePair<string, ActuatorState> module in stateStore.SnapshotBundle().Modules)
            {
                await serialModuleManager.SendTargetsAsync(module.Key, module.Value, requestAborted);
            }

            return JsonResponse(stateStore.SnapshotBundle());
        })
            .WithName("PostActuatorBundles")
            .Accepts<ActuatorStateBundle>("application/json")
            .Produces<ActuatorStateBundle>()
            .Produces(StatusCodes.Status400BadRequest);

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

    private static IResult JsonResponse(object value)
    {
        return Results.Content(JsonConvert.SerializeObject(value, JsonSettings), "application/json");
    }

    private static async Task<T?> ReadNewtonsoftJsonAsync<T>(HttpRequest request, CancellationToken cancellationToken)
    {
        using StreamReader reader = new(request.Body);
        string body = await reader.ReadToEndAsync(cancellationToken);
        return JsonConvert.DeserializeObject<T>(body, JsonSettings);
    }
}
