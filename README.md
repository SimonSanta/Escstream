# Escstream

Escstream, Multithreaded Real time Cooperative Wireless Embedded Circuits Video game.
Feb 2018 – Jun 2019


![Capture2](https://user-images.githubusercontent.com/17762123/120939633-3effe680-c719-11eb-84c3-6b2fedd9f409.PNG)
![Capture3](https://user-images.githubusercontent.com/17762123/120939638-43c49a80-c719-11eb-82fb-88feb286c53d.PNG)



Project in the scope of the course of [Design of Embedded and Real-Time Operating Systems](https://sites.uclouvain.be/archives-portail/cdc2020/en-cours-2020-lingi2315) from UCLouvain (Belgium). 

__Tasks:__ Per group of 4 the goal was to create from scratch a real time, synchronised cooperative puzzle game using a pair of DE10-Nano, MyPi-Nano and  Multi-Touch Screen.

In this context we used: 
* Intel Quartus Prime to program the FPGA using Verilog
* C for the game logic and display 
* Practiced Assembly (Baremetal)
* MicroAbassi multi threaded (RTOS) 
* Python for scripting the TCP communication protocols.

__Action:__ The tasks were mainly done as a group, as it was required that we all could be able to explain the code. In particular I focused my effort on the implementation of the TCP protocols and communication tools (we communicated via FLAGS bits)  as well as the main game design rules in C. The game had to be real time therefore interrupts methods were use along a scheduler.

__Result:__ We implemented a fully fledge cooperative puzzle game that is playable in real-time by two players using touch screens but added option such as Level selections and Pause menu.


*All files and presentation Powerpoint (In French) on Github.*
