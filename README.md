# Minimal Kodi Installation

Instructions for installing and configuring a minimal Kodi media center machine, to fill the gap left after the XBMC/Kodi-buntu distributions were retired. 

The Ubuntu-Desktop distribution is just too bloated for a "setup-once and forget about it" machine.


## Install a Minimal Ubuntu Dist.
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
  HOST=0.0.0.0
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
  
## OS Customization

- **Audio Configuration**

  If you're using an NVidia card or an NVidia ION box (ex Zotac Zbox) you'll soon realize that there's no audio in Kodi. 
  
  This is due to the fact that NVidia presents the wrong default device for audio.
  Reference: [NVidia HDMI Audio](http://http.download.nvidia.com/XFree86/gpu-hdmi-audio-document/index.html)
  - List all audio devices
    ```
    kodi@kodi:~$ sudo aplay -l
    **** List of PLAYBACK Hardware Devices ****
    Home directory not accessible: Permission denied
    card 0: Intel [HDA Intel], device 0: ALC888 Analog [ALC888 Analog]
      Subdevices: 1/1
      Subdevice #0: subdevice #0
    card 0: Intel [HDA Intel], device 1: ALC888 Digital [ALC888 Digital]
      Subdevices: 1/1
      Subdevice #0: subdevice #0
    card 1: NVidia [HDA NVidia], device 3: HDMI 0 [HDMI 0]
      Subdevices: 1/1
      Subdevice #0: subdevice #0
    card 1: NVidia [HDA NVidia], device 7: HDMI 0 [HDMI 0]
      Subdevices: 1/1
      Subdevice #0: subdevice #0
    card 1: NVidia [HDA NVidia], device 8: HDMI 0 [HDMI 0]
      Subdevices: 1/1
      Subdevice #0: subdevice #0
    card 1: NVidia [HDA NVidia], device 9: HDMI 0 [HDMI 0]
      Subdevices: 1/1
      Subdevice #0: subdevice #0
    ```
    
  - Get a list of all the devices with their `hw` name
    ```
    root@kodi:~# aplay -l  | grep card  | awk -F'[: ]' '{print "hw:" $2 "," $8}'
    hw:0,0
    hw:0,1
    hw:1,3
    hw:1,7
    hw:1,8
    hw:1,9
    ```
    
  - Test each device to figure out which one is actually producing audio
    ```
    sudo speaker-test -c 2 -r 48000 -D <HW-ID>
    ```
    
  - Once we determine which device is our audio device we can reconfigure pulseaudio to load it
    ```
    sudo echo "load-module module-alsa-sink device=<HW-ID>" >> /etc/pulse/default.pa
    ```
    
    The Zotac ZBOX ID40 uses hw:1,7
    ```
    sudo echo "load-module module-alsa-sink device=ht:1,7" >> /etc/pulse/default.pa
    ```
    
  - Reboot to apply changes and test
    Restarting pulse and Kodi should be enough but I haven't tested that.

- **Enable Kodi Power Control**
  
  By default Kodi lacks permissions to Suspend/Wake/Shutdown/Poweroff.
  Reference: [Kodi Wiki](http://kodi.wiki/view/HOW-TO:Suspend_and_wake_in_Ubuntu)
  - Install dependencies, they should all already be installed but better safe than sorry!
    ```
    sudo apt install policykit-1 upower acpi-support consolekit
    ```
  - Create file `/var/lib/polkit-1/localauthority/50-local.d/kodi.pkla` with the following content
    **NOTE:** The username `unix-user:kodi` will need to match your kodi user
    ```
    [Actions for kodi user]
    Identity=unix-user:kodi
    Action=org.freedesktop.upower.*;org.freedesktop.consolekit.system.*;org.freedesktop.udisks.*
    ResultAny=yes
    ResultInactive=yes
    ResultActive=yes
    ```
  - Restart polkit to apply the settings
    ```
    sudo systemctl restart polkitd
    ```

- **Enable USB automount**

  Configure it to use the USB label as the mount name.
  - Create file `/etc/udev/rules.d/11-media-by-label-auto-mount.rules` with the following content
    ```
    # Start at sdb to avoid system hard drive
    KERNEL!="sd[b-z][0-9]", GOTO="media_by_label_auto_mount_end"
     
    # Import FS info
    IMPORT{program}="/sbin/blkid -o udev -p %N"
     
    # Get a label if present, otherwise specify one
    ENV{ID_FS_LABEL}!="", ENV{dir_name}="%E{ID_FS_LABEL}"
    ENV{ID_FS_LABEL}=="", ENV{dir_name}="usbhd-%k"
     
    # Global mount options
    ACTION=="add", ENV{mount_options}="realtime"
     
    # Filesystem-specific mount options
    ACTION=="add", ENV{ID_FS_TYPE}=="vfat|ntfs",
    ENV{mount_options}="$env{mount_options},utf8,gid=100,umask=002"
     
    # Mount the device
    ACTION=="add", RUN+="/bin/mkdir -p /media/%E{dir_name}", RUN+="/bin/mount -o $env{mount_options} /dev/%k /media/%E{dir_name}"
     
    # Clean up after removal
    ACTION=="remove", ENV{dir_name}!="", RUN+="/bin/umount -l /media/%E{dir_name}", RUN+="/bin/rmdir /media/%E{dir_name}"
     
    # Exit
    LABEL="media_by_label_auto_mount_end"
    ```
    
- **Hide grub menu**

  We can hide the grub menu while still retaining the ability to use it by using the HIDDEN options
  - Edit `/etc/default/grub`
    ```
    GRUB_DEFAULT=0
    GRUB_HIDDEN_TIMEOUT=2
    GRUB_HIDDEN_TIMEOUT_QUIET=true
    GRUB_TIMEOUT=0
    GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`
    GRUB_CMDLINE_LINUX_DEFAULT="quiet splash"
    GRUB_CMDLINE_LINUX=""
    GRUB_GFXMODE=1920x1080
    GRUB_GFXPAYLOAD_LINUX=keep
    ```
    
  - Recreate it to apply settings
    ```
    sudo update-grub
    ```

- **Install Kodi Plymouth Theme**

  Plymouth is a Fedora project that enables graphical diplay during bootup. We're going to change the default theme to a Kodi one to make bootup look nicer.
  - Install dependencies
    ```
    sudo apt install fakeroot
    ```
  - Clone the git repo to a temp location
    ```
    git clone https://github.com/solbero/plymouth-theme-kodi-animated-logo.git /tmp/plymouth-theme-kodi
    ```
  - Navigate to repo and build the deb package
    ```
    cd /tmp/plymouth-theme-kodi
    ./build.sh
    ```
    
  - Install the package
    ```
    sudo dpkg -i /tmp/plymouth-theme-kodi/plymouth-theme-kodi-animated-logo.deb
    ```
    
  - Ensure Plymouth is enabled
    
    Verify that the `/etc/default/grub` contains
    - `GRUB_CMDLINE_LINUX` entry with `quiet` and `splash`
    - `GRUB_GFXMODE` with a value matching your display (ex `auto`)
    - `GRUB_GFXPAYLOAD_LINUX` with a value of `keep`
    
     
    If not edit it to match this
    ```
    GRUB_DEFAULT=0
    GRUB_HIDDEN_TIMEOUT=5
    GRUB_HIDDEN_TIMEOUT_QUIET=true
    GRUB_TIMEOUT=0
    GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`
    GRUB_CMDLINE_LINUX_DEFAULT="quiet splash"
    GRUB_CMDLINE_LINUX=""
    GRUB_GFXMODE=1920x1080
    GRUB_GFXPAYLOAD_LINUX=keep
    ```
    
    Apply the settings by regenerating grub
    ```
    sudo update-grub
    ```
    
  - Delete the source as we no longer need it
    ```
    rm -rf /tmp/plymouth-theme-kodi
    ```
    
  - Reboot to test
    ```
    sudo reboot
    ```
    
- **Enable Auto-Update**

  - Reference: [Ubuntu AutomaticSecurityUpdates](https://help.ubuntu.com/community/AutomaticSecurityUpdates)
  - Edit `/etc/apt/apt.conf.d/50unattended-upgrades`
  - Enable `"${distro_id}:${distro_codename}-updates";`
  - Exclude `lirc` from updates
  - Enable cleanup `Unattended-Upgrade::Remove-Unused-Dependencies "true";`
  - Enable Reboot `Unattended-Upgrade::Automatic-Reboot "true";`
  - Set Reboot time to 4am `Unattended-Upgrade::Automatic-Reboot-Time "04:00";`
    
  Final result should look something like this
  ```
  // Automatically upgrade packages from these (origin:archive) pairs
  Unattended-Upgrade::Allowed-Origins {
          "${distro_id}:${distro_codename}";
          "${distro_id}:${distro_codename}-security";
          // Extended Security Maintenance; doesn't necessarily exist for
          // every release and this system may not have it installed, but if
          // available, the policy for updates is such that unattended-upgrades
          // should also install from here by default.
          "${distro_id}ESM:${distro_codename}";
          "${distro_id}:${distro_codename}-updates";
  //      "${distro_id}:${distro_codename}-proposed";
  //      "${distro_id}:${distro_codename}-backports";
  };
  
  // List of packages to not update (regexp are supported)
  Unattended-Upgrade::Package-Blacklist {
          "lirc";
  //      "vim";
  //      "libc6";
  //      "libc6-dev";
  //      "libc6-i686";
  };
  
  // This option allows you to control if on a unclean dpkg exit
  // unattended-upgrades will automatically run
  //   dpkg --force-confold --configure -a
  // The default is true, to ensure updates keep getting installed
  //Unattended-Upgrade::AutoFixInterruptedDpkg "false";
  
  // Split the upgrade into the smallest possible chunks so that
  // they can be interrupted with SIGUSR1. This makes the upgrade
  // a bit slower but it has the benefit that shutdown while a upgrade
  // is running is possible (with a small delay)
  //Unattended-Upgrade::MinimalSteps "true";
  
  // Install all unattended-upgrades when the machine is shuting down
  // instead of doing it in the background while the machine is running
  // This will (obviously) make shutdown slower
  //Unattended-Upgrade::InstallOnShutdown "true";
  
  // Send email to this address for problems or packages upgrades
  // If empty or unset then no email is sent, make sure that you
  // have a working mail setup on your system. A package that provides
  // 'mailx' must be installed. E.g. "user@example.com"
  //Unattended-Upgrade::Mail "root";
  
  // Set this value to "true" to get emails only on errors. Default
  // is to always send a mail if Unattended-Upgrade::Mail is set
  //Unattended-Upgrade::MailOnlyOnError "true";
  
  // Do automatic removal of new unused dependencies after the upgrade
  // (equivalent to apt-get autoremove)
  Unattended-Upgrade::Remove-Unused-Dependencies "true";
  
  // Automatically reboot *WITHOUT CONFIRMATION*
  //  if the file /var/run/reboot-required is found after the upgrade
  Unattended-Upgrade::Automatic-Reboot "true";
  
  // If automatic reboot is enabled and needed, reboot at the specific
  // time instead of immediately
  //  Default: "now"
  Unattended-Upgrade::Automatic-Reboot-Time "04:00";
  
  // Use apt bandwidth limit feature, this example limits the download
  // speed to 70kb/sec
  //Acquire::http::Dl-Limit "70";
  ```