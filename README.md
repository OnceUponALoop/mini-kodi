# mini-kodi
Minimal Kodi Installation

TO-DO: Reconsider using MinimalCD since it doesn't support EFI. This could be a problem on NUCs and such.
For now if you don't have to dual boot then this should still be fine.

## Install Mini-Ubuntu
The goal is to install only what's absolutely necessary for kodi operation. 
A good candidate for this installation is [Ubuntu MinimalCD](https://help.ubuntu.com/community/Installation/MinimalCD) as it allows us to specify packages or package groups manually during installation.

### 1. Download ISO
    
Download the ISO that matches your architecture (ex 64-bit) from [Ubuntu MinimalCD](https://help.ubuntu.com/community/Installation/MinimalCD)
    
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

### 4. Configuration

Now that the OS is installed it's time to install everything else we need.
  - Log in as the user created earlier.
    
  - Update apt and initiate an upgrade `sudo apt update && sudo apt upgrade`
    
  - Install Xubuntu Core tasksel `sudo tasksel install xubuntu-core`
  
  - Add Kodi PPA Repository
  `sudo add-apt-repository ppa:team-xbmc/ppa`
  
  - Update apt and install kodi
  `sudo apt update && sudo apt install kodi`
  
  - Configure lightdm to start kodi on bootup
    Create file `/etc/lightdm/lightdm.conf` with the content:
    ```
    [Seat:*]
    autologin-user=kodi
    autologin-user-timeout=0
    autologin-session=kodi
    greeter-session=lightdm-gtk-greeter
    ```
    
  - Mount windows share, add the line to fstab
    ```
    # Mount PC Share
    //192.168.1.100/E$ /home/kodi/Storage cifs rw,_netdev,uid=kodi,gid=kodi,user=USER,password=PASS 0 0
    ```

    

  

    
