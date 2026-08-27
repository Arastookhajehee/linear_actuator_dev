mkdir C:\WSL
cp .\agb-robot-wsl-image.tar C:\WSL\agb-robot-wsl-image.tar
cd C:\WSL
wsl --install
wsl --import robot .\ .\mkdir C:\WSL\agb-robot-wsl-image.tar
wsl -d robot exit
echo "Run command on windows terminal 1: `wsl -d robot` -->> bash t1.sh"
echo "Run command on windows terminal 2: `wsl -d robot` -->> bash t2.sh"
