// Grasshopper Script Instance
#region Usings
using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
#endregion

public class Script_Instance : GH_ScriptInstance
{
    private void RunScript(
        string url,
        List<int> targets,
        bool submit,
        ref object a,
        ref object b,
        ref object c)
    {
        if (!submit)
        {
            a = "Idle (set submit=true to send)";
            b = null;
            c = null;
            return;
        }

        if (string.IsNullOrWhiteSpace(url))
        {
            a = "Error";
            b = null;
            c = "URL input is required.";
            return;
        }

        if (targets is null || targets.Count < 4)
        {
            a = "Error";
            b = null;
            c = "Targets must contain at least 4 values: a1..a4.";
            return;
        }

        try
        {
            var body = new
            {
                a1_target = targets[0],
                a2_target = targets[1],
                a3_target = targets[2],
                a4_target = targets[3],
            };

            string requestBody = JsonSerializer.Serialize(body);
            var result = PostTargetsAsync(url, requestBody).GetAwaiter().GetResult();
            a = "Sent";
            b = result.statusCode;
            c = result.responseBody;
        }
        catch (Exception ex)
        {
            a = "Error";
            b = null;
            c = ex.Message;
        }
    }

    private static async Task<(int statusCode, string responseBody)> PostTargetsAsync(
        string url,
        string requestBody
    )
    {
        using var httpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(15) };
        using var content = new StringContent(requestBody, Encoding.UTF8, "application/json");
        using HttpResponseMessage response = await httpClient.PostAsync(url, content);
        string responseBody = await response.Content.ReadAsStringAsync();
        return ((int)response.StatusCode, responseBody);
    }
}

