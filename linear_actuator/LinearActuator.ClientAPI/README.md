# LinearActuator.ClientAPI

Typed C# client generated from the running backend OpenAPI document.

Generation requires the WPF backend API to be running at `http://127.0.0.1:7500`.

```powershell
dotnet run --project ..\LinearActuator.App
```

Start the API from the WPF UI, then run:

```powershell
dotnet build /t:GenerateClient
```

The generated client is written to `Generated/LinearActuatorApiClient.cs`.
