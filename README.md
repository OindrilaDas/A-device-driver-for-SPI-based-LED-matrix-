# A-device-driver-for-SPI-based-LED-matrix-


1) Give power to the board and connect ethernet and USB from host to the board.

 
2) Connect the LED matrix to VCC, GND and IO11(Din), I012(CS), I013(CLK) IO ports of the Galileo Gen2 board using jumper wires and breadboard.
 Similarly, connect the Ultrasonic sensor to VCC, IO2(Trigger), IO3(Echo), GND of the Galileo Gen 2 Board.
 

3) Now give the command "ls /dev" and check which USB is in use. Suppose, ttyUSB0 is used. Give permission to open the device from the host terminal by typing the 
command "sudo chmod 777 /dev/ttyUSB0". Open putty and give the serial name dev/ttyUSB0 and baud rate 115200. In the putty terminal give login and root to enter.


4) Check whether the IP address of host terminal and serial terminal are in the same network , by giving the command "ifconfig" in both of them. 
If they are not in the same network, configure putty(target) by using the command "ifconfig enp0s20f6 192.168.1.5 netmask 255.255.0.0 up".
 Note: the last digit of the IP address for host should be different from that of the target.


5) To set the target environment, type "sudo su" in the command terminal and enter the directory where the files are kept using cd command. 
To create the executable files, give command "make" in the host terminal to run the Makefile.


6) To copy the executable files from host to target use the command "scp filename root@serialterminal-IP:/home" in the host terminal.


7) In the putty terminal, enter the home directory, using "cd /home" command. Check if the file is copied using "ls" command.


8) Remove the already initialized spidev, by running the command "rmmod spidev" on the target.
 

9) To initialize the driver give the command "insmod filename.ko" on putty, then give the command " ./filename".


10) Give the command " ./filename" to execute the file.


11) A moving car sequence would be seen in the LED 8X8 matrix after file execution. The threshold values for the distance of an interference is set to be 175 mm,
 350 mm & 550mm. If the interference(object) is moving away from ultrasonic sensor and its distance from sensor is above threshold value(550 mm) then the speed at
 which the car moves is slow. If the object moves towards the sensor and its distance is lesser than that of the threshold(350 mm), then the car moves at a higher speed
 and shifts to its right. When the distance between object and the sensor is less than threshold(175 mm), the car's speed increases further and swerves right. 
When the object crosses threshold(175 mm), the car crashes into its right side.


11) The execution stops in 80 seconds. To stop execution before 80 seconds, press Ctrl+C. Do not press Ctrl+Z.

