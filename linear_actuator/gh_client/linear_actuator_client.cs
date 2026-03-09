// Grasshopper Script Instance
#region Usings
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using Grasshopper.Kernel.Data;
using Grasshopper;
#endregion

public class Script_Instance : GH_ScriptInstance
{
    private static readonly HttpClient HttpClient = new HttpClient
    {
        Timeout = TimeSpan.FromSeconds(15)
    };

    private void RunScript(
		DataTree<string> url,
		DataTree<int> valueTree,
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


        if (valueTree is null || valueTree.BranchCount == 0)
        {
            a = "Error";
            b = statusCodes;
            responses.Add("valueTree must contain at least one branch.", new GH_Path(0));
            c = responses;
            return;
        }

        int successCount = 0;

        try
        {
            foreach (GH_Path path in valueTree.Paths)
            {
                List<int> branch = valueTree.Branch(path);
                if (branch is null || branch.Count < 4)
                {
                    statusCodes.Add(-1, path);
                    responses.Add("Branch must contain at least 4 target values.", path);
                    continue;
                }

                var body = new
                {
                    a1_target = branch[0],
                    a2_target = branch[1],
                    a3_target = branch[2],
                    a4_target = branch[3],
                };
                var endpoint = url.Branch(path)[0];

                        if (string.IsNullOrWhiteSpace(endpoint))
                        {
                            a = "Error";
                            b = statusCodes;
                            responses.Add("URL input is required.", new GH_Path(0));
                            c = responses;
                            return;
                        }

                string requestBody = JsonSerializer.Serialize(body);
                var result = PostTargets(endpoint, requestBody);

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

    private static (int statusCode, string responseBody) PostTargets(
        string url,
        string requestBody
    )
    {
        using var content = new StringContent(requestBody, Encoding.UTF8, "application/json");
        using HttpResponseMessage response = HttpClient.PostAsync(url, content).Result;
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
        // If response is not valid JSON, return original text
        return json;
    }
}
}

