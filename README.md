# mini-kodi
Minimal Kodi Installation

TO-DO: Reconsider using MinimalCD since it doesn't support EFI. This could be a problem on NUCs and such.
For now if you don't have to dual boot then this should still be fine.

## Install Mini-Ubuntu
The goal is to install only what's absolutely necessary for kodi operation. 

[Ubuntu MinimalCD](https://help.ubuntu.com/community/Installation/MinimalCD) was chosen initially as it allows us to specify packages or package groups manually during installation but it doesn't support EFI out of the box and requires an active internet connection during installation as the CD doesn't contain any.

[Ubuntu Server](https://www.ubuntu.com/download/server) seemed like the next logical choice. It has very minimal defaults, this should allow us to only add the packages we need.

### 1. Download ISO
    
Download the ISO that matches your architecture (ex 64-bit) from [Ubuntu](https://www.ubuntu.com/download/server).
I find the torrent option is usually the fastest.

Note: Chose the LTS version for longest lifetime.
    
### 2. Boot from ISO

Burn the ISO to a DVD or use a tool like [UNetBootIn](https://unetbootin.github.io/) or [Rufus](https://rufus.akeo.ie/) to create a boot USB.
    
### 3. Go through the installation

  - Select `Install Ubuntu Server`
  - Select Language Option(s)
  - Assign a hostname

    Note: Something logical (kodi, media-pc, etc...) that doesn't conflic with existing hosts on LAN.

  - Select Ubuntu Mirror Options (ex. Location/Proxy)

  - Create a user account

    | Real Name | User name | Password |
    | --------- | --------- | -------- |
    | kodi      | kodi      | ******** |

  - Don't encrypt home directory 

    Introduces complexity, enable only if you're equiped to handle it :)

  - Set the timezone if auto-detected one isn't correct

  - Partition Disk

    Use `Guided - use entire disk` unless you want to manually set it up

  - Write changes to disk

  - Configure Updates
  
    Set to `Install security updates automatically` unless you have a reason to manage it manually.
    
  - Software Selection
  
    This is where we define what we want to install
    
      - `Standard System Utilities`
      - `OpenSSH Server`


## Install NVidia Driver
The test machine i'm using is a [Zotac ID40+](https://www.zotac.com/us/product/mini_pcs/id40-plus) with an NVidia ION GT218 chipset.

I checked the NVidia site for the latest available driver release and it indicated **340.102**.

- Add the graphics drivers PPA
  ```
  sudo add-apt-repository ppa:graphics-drivers/ppa
  ```
  
- Install the recommended version
  ```
  sudo apt update && sudo apt install nvidia-340 vdpauinfo
  ```

## Install Kodi
Now that the OS is installed it's time to install everything else we need.

- Log in as the user created earlier.
  
- Update apt and initiate an upgrade 
  ```
  sudo apt update && sudo apt upgrade
  ```
  
- Install Xubuntu Core
  This will install our minimal X environment on top of which we'll run kodi 
  ```
  sudo tasksel install xubuntu-core
  ```

- Add Kodi PPA Repository
  ```
  sudo add-apt-repository ppa:team-xbmc/ppa
  ```
  
- Update apt and install kodi
  ```
  sudo apt update && sudo apt install kodi
  ```
  
- Configure lightdm to start kodi on bootup
  
  Create file `/etc/lightdm/lightdm.conf.d/kodi.conf`
  ```
  [Seat:*]
  autologin-user=kodi
  autologin-user-timeout=0
  autologin-session=kodi
  greeter-session=lightdm-gtk-greeter
  ```
  
- Test it by launching lightdm
  ```
  sudo systemctl start ligthdm
  ```
  If it has already started, test it by killing lightdm, it should restart with Kodi
  ```
  sudo killall lightdm
  ```

## Configure XBOX DVD Kit Remote
I have an old XBOX DVD kit receiver that I like using. It's getting complicated to keep it working as it seems lirc dropped support for it. 
If you've got a different remote/receiver check the [Kodi wiki](http://kodi.wiki/view/HOW-TO:Set_up_LIRC) for setup instructions.

- Install useful stuff
  ```
  sudo apt install git vim-nox
  ```

- Clone this repo to user home
  ```
  git clone https://github.com/OnceUponALoop/mini-kodi.git $HOME/mini-kodi
  ```

- Copy lirc source
  Keeping it in `/usr/local/src` since we'll need to keep coming back to it on kernel updates.
  ```
  sudo cp -r /$HOME/mini-kodi/lirc-0.9.0 /usr/local/src
  ```

- Set the directory permissions to allow your non-root user access
  ``` bash
  sudo chown -R $(id -un):$(id -gn) /usr/local/src/lirc-0.9.0
  ```

- Install required dependencies for building
  ```
  sudo apt install -y lirc dialog automake autoconf libtool
  ```
  When prompted for the lirc dpkg setup just choose `None/None`
  
- Prepare build
  ```
  cd /usr/local/src/lirc-0.9.0
  ./autogen.sh
  ./configure --with-driver=xbox
  ```

- Make and install
  ```
  cd /usr/local/src/lirc-0.9.0/drivers/lirc_xbox
  make
  sudo make install
  ```
  
- Copy the configuration files
  ```
  sudo cp $HOME/mini-kodi/config-files/*.conf /etc/lirc/
  ```
  
- Copy the kernel post-install script
  This script will rebuild the lirc_xbox module when a kernel is updated
  ```
  sudo cp $HOME/mini-kodi/config-files/lirc-module-rebuild /etc/kernel/postinst.d
  sudo chmod 755 /etc/kernel/postinst.d/lirc-module-rebuild
  ```

- (Optional) Test it by reinstalling the current kernel
  ```
  sudo apt install --reinstall linux-image-$(uname -r)
  ```
  
- Restart lirc & kodi to test functionality
  ```
  sudo udevadm trigger
  sudo systemctl restart lirc
  killall -HUP kodi.bin
  ```

## Configure IRExec
We can use the lirc irexec binary in daemon mode to set up remote control combo actions.

In my example i'm using it to restart kodi and remount the network shares, it comes in handy as a quick restart if kodi hangs.

- Create service file `/etc/systemd/system/irexec-root.service` 
  ```
  sudo cp $HOME/mini-kodi/config-files/irexec-root.service /etc/systemd/system/
  ```
  
- Create the configuration file that defines the combo and actions
  In this example i'm using jennys number `867-5309` to restart kodi and remount the drives
  ```
  sudo cp $HOME/mini-kodi/config-files/lircrc.conf /etc/lirc/
  ```
  
- Enable and start the irexec service
  ```
  sudo systemctl enable irexec-root.service
  sudo systemctl start irexec-root.service
  ```

## Install SABNzbd
Reference: [SABnzbd Documentation](https://sabnzbd.org/wiki/installation/install-ubuntu-repo)

- Install sabnzbd repositories
  ``` 
  sudo add-apt-repository ppa:jcfp/nobetas
  sudo add-apt-repository ppa:jcfp/sab-addons
  ```
  
- Install sabnzbd and dependencies
  ```
  sudo apt update && sudo apt install sabnzbdplus software-properties-common python-sabyenc par2-tbb
  ```  
  
- Enable auto-startup
  ```
  sudo systemctl enable sabnzbdplus
  ```

- Edit the configuration file `/etc/default/sabnzbdplus`
  ```
  # [required] user or uid of account to run the program as:
  USER=kodi
  
  # [optional] full path to the configuration file of your choice;
  #            otherwise, the default location (in $USER's home
  #            directory) is used:
  CONFIG=/home/kodi/.config/sabnzbd/sabnzbd.ini
  
  # [optional] hostname/ip and port number to listen on:
  HOST=
  PORT=8081
  
  # [optional] extra command line options, if any:
  EXTRAOPTS=
  ```

- Start it
  ```
  sudo systemctl start sabnzbdplus
  ```
  
## Install Sonarr

- Install Mono Repositories
  ```
  sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
  echo "deb http://download.mono-project.com/repo/ubuntu xenial main" | sudo tee /etc/apt/sources.list.d/mono-official.list
  ```

- Install Mono
  ```
  sudo apt update && sudo apt install libmono-cil-dev curl mediainfo
  ```
  
- Install Sonarr Repository
  ```
  sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys FDA5DFFC
  sudo echo "deb http://apt.sonarr.tv/ master main" | sudo tee /etc/apt/sources.list.d/sonarr.list
  ```
  
- Install Sonarr
  ```
  sudo apt update && sudo apt install nzbdrone
  ```

- Create a systemd service for Sonarr
  **Note:** The user and pathing in the service file might need to be adjusted to match your system/user info.
  ```
  sudo cp $HOME/mini-kodi/config-files/sonarr.service /etc/systemd/system/
  ```
  
- Enable and start it
  ```
  sudo systemctl enable sonarr.service
  sudo systemctl start  sonarr.service
  ```
  
## Install Radarr
There's still no repo packages available for radarr so we'll have to install it manually. This isn't too bad especially since it auto-updates anyway.

- Install Radarr
  ```
  cd /opt
  sudo wget $( curl -s https://api.github.com/repos/Radarr/Radarr/releases | grep linux.tar.gz | grep browser_download_url | head -1 | cut -d \" -f 4 )
  sudo tar -xvzf Radarr.develop.*.linux.tar.gz
  sudo rm -f /opt/Radarr.develop.*.linux.tar.gz
  ```
  
- Create a systemd service for Sonarr
  **Note:** The user and pathing in the service file might need to be adjusted to match your system/user info.
  ```
  sudo cp $HOME/mini-kodi/config-files/radarr.service /etc/systemd/system/
  ```
  
- Enable and start it
  ```
  sudo systemctl enable radarr.service
  sudo systemctl start  radarr.service
  ```