# Installation Steps

1. Download the WSL Robot Server image from this link

   * [WSL Robot Server Image](https://drive.google.com/file/d/1NZd80ZRaaY-qoB39EwnkRCC1t4XJJ57P/view?usp=drive_link)
   * Put the file **HERE in THIS**  folder beside the `install.ps1` script
2. Launch PowerShell in admin mode **HERE** by `Shift + Right Click`

   * Run the `./install.ps1` script by typing the command in powershell
   * Run the following commands:
     * `wsl --import robot .\ C:\WSL\agb-robot-wsl-image.tar`
     * `wsl -d robot`
     * `exit`
3. Run the following commands in separate windows terminals one by one

   * Terminal A
     * `wsl -d robot`
     * `bash t1.sh`
   * Terminal B
     * `wsl -d robot`
     * `bash t2.sh`
4. Open Rhino and open the [Robot IK File](./craine_ik.3dm)
5. Launch Grasshopper and open the [GH Robot Client](./gh_client.gh)
6. Send Requests for robot motion solutions

> [!NOTE]
> The Default URL for the robot is `http://127.0.0.1:8000`
> We can test the server's status from `http://127.0.0.1:8000/health`
> We can change the URL inside the **Requester Client C# Component** in Grasshopper
