// Grasshopper Script Instance
#region Usings
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text.Json;
using Grasshopper.Kernel.Data;
#endregion

public class Script_Instance : GH_ScriptInstance
{
    private static readonly HttpClient HttpClient = new HttpClient
    {
        Timeout = TimeSpan.FromSeconds(15)
    };

    private void RunScript(
        DataTree<string> url,
        bool submit,
        ref object a,
        ref object b,
        ref object c)
    {
        var statusCodes = new DataTree<int>();
        var responses = new DataTree<string>();

        if (!submit)
        {
            a = "Idle (set submit=true to send)";
            b = statusCodes;
            c = responses;
            return;
        }

        if (url is null || url.BranchCount == 0)
        {
            a = "Error";
            b = statusCodes;
            responses.Add("url tree must contain at least one branch.", new GH_Path(0));
            c = responses;
            return;
        }

        int successCount = 0;

        try
        {
            foreach (GH_Path path in url.Paths)
            {
                List<string> branch = url.Branch(path);
                if (branch is null || branch.Count == 0 || string.IsNullOrWhiteSpace(branch[0]))
                {
                    statusCodes.Add(-1, path);
                    responses.Add("Branch must contain a non-empty URL.", path);
                    continue;
                }

                string endpoint = branch[0];
                var result = GetActuators(endpoint);

                statusCodes.Add(result.statusCode, path);
                responses.Add(FormatJsonPretty(result.responseBody), path);
                successCount++;
            }

            a = successCount > 0 ? "Sent" : "No valid branches";
            b = statusCodes;
            c = responses;
        }
        catch (Exception ex)
        {
            a = "Error";
            b = statusCodes;
            responses.Add(ex.Message, new GH_Path(0));
            c = responses;
        }
    }

    private static (int statusCode, string responseBody) GetActuators(string url)
    {
        using HttpResponseMessage response = HttpClient.GetAsync(url).Result;
        string responseBody = response.Content.ReadAsStringAsync().Result;
        return ((int)response.StatusCode, responseBody);
    }

    private static string FormatJsonPretty(string json)
    {
        if (string.IsNullOrWhiteSpace(json))
        {
            return json ?? string.Empty;
        }

        try
        {
            using JsonDocument doc = JsonDocument.Parse(json);
            return JsonSerializer.Serialize(doc.RootElement, new JsonSerializerOptions
            {
                WriteIndented = true
            });
        }
        catch
        {
            return json;
        }
    }
}
