# Installing WSL Ubuntu 22.04

From [[Robotics]]

This guide helps you install **Ubuntu 22.04 inside Windows** using **WSL**, then prepare it so it is ready for [[How to install ros2_humble]] and [[MoveIt Installation overall]].

If you have never used Linux before, that is completely fine. This tutorial assumes nothing.

## What WSL Actually Is

**WSL** stands for **Windows Subsystem for Linux**.

In plain English, WSL lets you run a real Linux environment inside Windows without needing to:

- erase Windows
- install a second operating system
- make a virtual machine by hand
- buy another computer

Think of it like this:

- **Windows** is still your main operating system
- **Ubuntu** runs inside it as a Linux environment
- You open Ubuntu in a terminal window and work there just like a Linux machine

For robotics, ROS 2, and MoveIt, this matters because a lot of tools are designed first for **Ubuntu Linux**. WSL gives you a practical way to follow Linux-based robotics tutorials while still using your Windows laptop.

## What Ubuntu Is

**Ubuntu** is a popular version of Linux.

Linux is an operating system family, just like Windows is an operating system. Ubuntu is one of the most common Linux distributions used for programming, robotics, and servers.

When people say things like:

- "open a terminal"
- "run `sudo apt update`"
- "install ROS 2 on Ubuntu 22.04"

they are usually talking about working in this Linux environment.

## Why Ubuntu 22.04 Specifically

ROS 2 Humble is built for **Ubuntu 22.04**. That is why this guide uses that exact version.

If you install another Ubuntu version, some commands in later tutorials may fail or install the wrong packages.

## What You Will End Up With

By the end of this guide, you will have:

- WSL enabled on Windows
- Ubuntu 22.04 installed
- a Linux username and password created
- package lists updated
- basic tools installed
- WSL configured well enough to continue with ROS 2 and MoveIt

## Before You Start

You need:

- a Windows 10 or Windows 11 machine
- an internet connection
- permission to use an administrator terminal on Windows
- enough free disk space

If you are on a work or school machine with strict IT restrictions, installation may be blocked.

## Step 1: Open Windows PowerShell as Administrator

You need to start from a Windows terminal with administrator rights.

1. Click the **Start** button.
2. Type `PowerShell`.
3. Right-click **Windows PowerShell**.
4. Choose **Run as administrator**.

You may see a Windows security prompt asking if you want to allow changes. Click **Yes**.

## Step 2: Install WSL

In the administrator PowerShell window, run:

```powershell
wsl --install
```

What this usually does:

- enables the Windows features WSL needs
- installs the latest WSL system components
- installs a default Linux distribution

On many machines, this command is enough by itself.

## Step 3: Restart Windows If Asked

If Windows tells you to reboot, restart your computer.

This is normal. Some Windows features only finish enabling after a reboot.

After the restart, open **PowerShell** again. You do not usually need administrator mode for the next checks.

## Step 4: Check Whether WSL Is Installed Correctly

Run:

```powershell
wsl --status
```

You should see information showing WSL is installed.

Then run:

```powershell
wsl -l -v
```

This command lists installed Linux distributions and their WSL version.

Important idea:

- **WSL 1** is the older version
- **WSL 2** is the newer version and is the one you want

For ROS 2 and MoveIt work, **WSL 2** is the right choice.

## Step 5: Install Ubuntu 22.04 Specifically

Even if `wsl --install` already added some Linux distribution, install the exact one you want:

```powershell
wsl --install -d Ubuntu-22.04
```

If it says the distribution is already installed, that is fine.

If you want to see all available Linux distributions, you can run:

```powershell
wsl --list --online
```

## Step 6: Launch Ubuntu for the First Time

Open Ubuntu in one of these ways:

- search for `Ubuntu 22.04` in the Start menu and open it
- or run this in PowerShell:

```powershell
wsl -d Ubuntu-22.04
```

The first launch may take a minute or two.

You will then be asked to create:

- a **Linux username**
- a **Linux password**

This is separate from your Windows account.

### Important password note

When you type your Linux password, **nothing appears on screen**. No dots. No stars. No cursor movement.

This is normal in Linux.

Type the password carefully and press Enter.

## Step 7: Understand the Prompt You Are Seeing

After setup, you will see something like this:

```bash
yourname@yourcomputer:~$
```

This is the Linux shell prompt.

What the parts mean:

- `yourname` is your Linux username
- `yourcomputer` is the machine name
- `~` means your home folder
- `$` means the shell is ready for a command

This is where you will do most ROS 2 and MoveIt work later.

## Step 8: Update Ubuntu's Package Lists

Ubuntu installs software from online package repositories. Before installing anything else, refresh the package list.

Run inside **Ubuntu**, not PowerShell:

```bash
sudo apt update
```

What this means:

- `sudo` means "run this with administrator privileges inside Linux"
- `apt` is Ubuntu's package manager
- `update` refreshes the list of available software

It will ask for your Linux password.

## Step 9: Upgrade Installed Packages

Now bring the installed software up to date:

```bash
sudo apt upgrade -y
```

This may take a while.

## Step 10: Install Basic Useful Tools

These tools are not all strictly required for WSL itself, but they are useful immediately and help with later ROS 2 and MoveIt setup.

Run:

```bash
sudo apt install -y curl git wget software-properties-common ca-certificates gnupg lsb-release python3 python3-venv python3-pip
```

What these tools are for:

- `curl` and `wget`: download things from the internet
- `git`: download code repositories and track changes
- `software-properties-common`: helps manage repositories
- `ca-certificates` and `gnupg`: help verify secure package sources
- `lsb-release`: helps identify your Ubuntu version

## Step 11: Confirm You Really Are on Ubuntu 22.04

Run:

```bash
lsb_release -a
```

You want to see something like:

```text
Description:    Ubuntu 22.04.x LTS
```

This matters because the next robotics tutorials expect Ubuntu 22.04.

## Step 12: Confirm WSL 2 From Windows

Back in **PowerShell**, run:

```powershell
wsl -l -v
```

You want your Ubuntu distribution to show **Version 2**.

If it shows version 1, convert it with:

```powershell
wsl --set-version Ubuntu-22.04 2
```

## Step 13: Learn the Difference Between Windows and Ubuntu Paths

This is one of the most confusing things for beginners, so it is worth explaining early.

### In Windows, paths look like this

```text
C:\Users\YourName\Documents
```

### In Linux, paths look like this

```text
/home/yourname
```

Inside Ubuntu:

- your personal folder is usually `/home/yourname`
- you can shorten that to `~`

So these two mean the same thing inside Ubuntu:

```bash
cd /home/yourname
cd ~
```

## Step 14: Understand Where to Keep Your Robotics Work

For Linux tools like ROS 2 and MoveIt, it is usually best to keep your Linux projects **inside the Linux filesystem**, for example in your home folder:

```bash
~/ws_moveit2
~/ws_robot
```

That tends to be simpler and more reliable than working out of Windows folders.

## Step 15: Test a Few Basic Linux Commands

If Linux is completely new, run these small commands to get comfortable:

```bash
pwd
ls
mkdir -p ~/test_folder
cd ~/test_folder
pwd
cd ~
rm -r ~/test_folder
```

What they mean:

- `pwd`: print working directory, shows where you are
- `ls`: list files and folders
- `mkdir`: make a folder
- `cd`: change folder
- `rm -r`: remove a folder and its contents

## Step 16: Make Sure Ubuntu Starts Cleanly Next Time

Close the Ubuntu window.

Then open it again from the Start menu and make sure:

- it opens without errors
- you reach the Linux prompt
- your username looks correct

This quick reopen test catches setup problems early.

## Step 17: Optional But Helpful WSL Commands

These are useful to know later.

### Shut down all WSL instances

Run in PowerShell:

```powershell
wsl --shutdown
```

This fully stops the Linux environment. It is useful if WSL gets into a strange state.

### Open Ubuntu directly

```powershell
wsl -d Ubuntu-22.04
```

### See installed distributions

```powershell
wsl -l -v
```

## Common Beginner Questions

### "Am I inside Windows or Linux right now?"

If your terminal prompt looks like this:

```bash
yourname@yourcomputer:~$
```

you are in Linux.

If the prompt looks more like PowerShell, such as:

```powershell
PS C:\Users\YourName>
```

you are in Windows.

### "Why does `sudo` ask for a password?"

Because it is asking for administrator permission inside Ubuntu. This is normal and expected.

### "Why does typing my password show nothing?"

That is standard Linux behavior. The terminal hides password input completely.

### "Did I install a second operating system?"

Not in the traditional dual-boot sense. Windows is still your main operating system. WSL gives you a Linux environment running within Windows.

### "Do I need to learn all of Linux before continuing?"

No. You only need a small set of commands at first. You can learn the rest gradually while following the robotics tutorials.

## Troubleshooting

### `wsl` command is not recognized

Try installing pending Windows updates, then restart and try again.

If needed, install WSL from an administrator PowerShell window:

```powershell
wsl --install
```

### Ubuntu does not start after installation

Try:

```powershell
wsl --shutdown
wsl -d Ubuntu-22.04
```

### The distribution shows WSL version 1 instead of 2

Run:

```powershell
wsl --set-version Ubuntu-22.04 2
```

### `sudo apt update` fails because of network or repository errors

Wait a minute and try again. Temporary mirror or network failures do happen.

If your machine is on a corporate or school network, firewall restrictions may also be involved.

## Ready for the Next Tutorials

If all of the following are true, your system is ready for the next step:

- `wsl -l -v` shows `Ubuntu-22.04` on **Version 2**
- Ubuntu opens successfully
- `lsb_release -a` reports **Ubuntu 22.04**
- `sudo apt update` works
- `sudo apt upgrade -y` works
- basic tools installed successfully

Next guides:

- [[How to install ros2_humble]]
- [[MoveIt Installation overall]]

## Quick Copy-Paste Checklist

### In Administrator PowerShell

```powershell
wsl --install
wsl --install -d Ubuntu-22.04
```

### In Ubuntu 22.04

```bash
sudo apt update
sudo apt upgrade -y
sudo apt install -y curl git wget software-properties-common ca-certificates gnupg lsb-release
lsb_release -a
```

### Back in PowerShell

```powershell
wsl -l -v
```

You are now ready to continue with ROS 2 Humble and MoveIt setup.
