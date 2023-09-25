First Lab requires to output "#hello world#".  
===  
The VM must install in VirtualBox in order to read the SD card. I tried VMWare and it failed.  
---  
Login Loop?  
A: My guess is that it's due to the system PATH variable being modified, and you didn't restore it before turning off the computer.  
  Use Ctrl + Alt + F3 into the shell(username:root, password:yourpassword), restore(you may find that many command become invalid, I used emacs that still worked normal to modify the file) the /etc/environment file to what it should be. Then reboot.  
