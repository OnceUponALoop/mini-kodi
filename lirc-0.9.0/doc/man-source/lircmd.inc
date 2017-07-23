[NAME]
lircmd - LIRC mouse daemon translates infrared signals into mouse events

[DESCRIPTION]
This daemon can simulate a MouseSystems, IntelliMouse or IMPS/2 type mouse. It
gets the received buttons from lircd and converts them to mouse events. To
make this possible, lircmd needs a config file located
in /etc/lircmd.conf. In this file you have to determine which
button from which remote causes a mouse move or a mouse button click.
You can also specify a special button which activates or deactivates the mouse
mode.

[OPTIONS]
If you provide the \-\-nodaemon option lircmd won't fork to background.

On Linux systems the \-\-uinput option will enable automatic generation
of Linux input events. lircmd will open /dev/input/uinput and inject
the simulated mouse events into the Linux kernel rather than creating
the /dev/lircm device.
[FILES]
lircmd will use syslogd to output error messages. It depends on your
system configuration where they will show up.

