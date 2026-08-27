# Installation Steps

1. Download the WSL Robot Server image from this link
    - [WSL Robot Server Image](https://drive.google.com/file/d/1NZd80ZRaaY-qoB39EwnkRCC1t4XJJ57P/view?usp=drive_link)
    - Put the file **HERE in THIS**  folder
3. run the `./install.ps1` script
4. run the following commands in separate windows terminals one by one
    - Terminal A
        - `wsl -d robot`
        - `bash t1.sh`
    - Terminal B
        - `wsl -d robot`
        - `bash t2.sh`
5. Open Rhino and open the [Robot IK File](./craine_ik.3dm)
6. Launch Grasshopper and open the [GH Robot Client](./gh_client.gh)
7. Send Requests for robot motion solutions

> [!NOTE]
> The Default URL for the robot is `http://127.0.0.1:8000`
> We can test the server's status from `http://127.0.0.1:8000/health`
> We can change the URL inside the **Requester Client C# Component** in Grasshopper
