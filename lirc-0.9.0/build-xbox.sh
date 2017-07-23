echo "Installing packages required for compiling..."
apt-get build-dep -y lirc
sudo apt-get install -y dialog automake autoconf libtool
echo "Installing packages complete - Starting build..."
echo "Running pre-compile scripts..."
./autogen.sh
./configure -with-driver=userspace
echo "Compiling Lirc driver: lirc_xbox"
cd drivers/lirc_xbox/
make
echo "Installing newly compiled driver...."
sudo make install
echo "Installation complete"
sleep 3
echo "blacklisting xpad driver..."
echo "blacklist xpad" >> /etc/modprobe.d/blacklist.conf
echo "installing lirc"
sudo apt-get install -y lirc
echo "lirc install complete"
cd ../..
cp -r hardware.conf /etc/lirc/
cp -r lircd.conf /etc/lirc/
echo "*** Drivers have now been compiled and installed, xpad driver blacklisted, configuration files copied, and lirc installed. Have a nice day ***"

