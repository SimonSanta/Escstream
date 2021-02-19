#import for IP
import socket
#import for SPI
import RPi.GPIO as GPIO
from time import sleep
import spidev
#import for Threads
import sys
from threading import Thread
from time import sleep


#function to convert a list of 4 2hex values to an integer
def hexltoint(l):
  return (l[0]*16777216+l[1]*65536+l[2]*256+l[3])

#function to convert an integer to a list of 4 2hex values
def inttohexl(val):
  l1=val%256
  l2=(val%65536)-l1
  l3=(val%16777216)-l2-l1
  l4=val-l3-l2-l1
  return [l4,l3,l2,l1]
  
#Definition of the 2 used threads
class Sender(Thread):
  def __init__(self,s):
    Thread.__init__(self)
    self.socket=s
    self.FromSPIval=0
    self.LastFromSPIval=0
  def run(self):
    while True:
      if self.FromSPIval != self.LastFromSPIval:
        print('FromSPI : ',self.FromSPIval)
        self.LastFromSPIval=self.FromSPIval
        FromSPIvalstr=str(self.FromSPIval)
        mes=FromSPIvalstr.encode()
        self.socket.send(mes)
    
class Receiver(Thread):
  def __init__(self,s):
    Thread.__init__(self)
    self.socket=s
    self.ToSPIval=0
  def run(self):
    while True:
      rec=self.socket.recv(1024)
      try:
        self.ToSPIval=int(rec.decode())
      except:
        pass
#SETUP OF THE SPI
MySPI_FPGA = spidev.SpiDev()
MySPI_FPGA.open(0,0)
MySPI_FPGA.max_speed_hz = 500000

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

#SETUP OF THE WIFI

me='10.3.141.1'
other='10.3.141.106'
ports=5561 #envoie sur le port 5561
portr=5560 #recoie sur le port 5560

ss = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sr = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print("Socket created.")
try:
    sr.bind(('', portr))
except socket.error as msg:
    print(msg)
    
sr.listen(1)
sr,lol=sr.accept()
ss.connect((other,ports))
#s.setblocking(True)
print('Got connection from',other)

#Active part
FromSPI=[0x00, 0x00, 0x00, 0x00]
ToSPI=[0x00, 0x00, 0x00, 0x00]
ToSPIval=0
FromSPIval=0
LastFromSPIval=0
LastToSPIval=0

sender=Sender(sr)
receiver=Receiver(ss)


receiver.start()
sender.start()

while True:
  if receiver.ToSPIval != LastToSPIval:
    LastToSPIval=receiver.ToSPIval
    ToSPI=inttohexl(receiver.ToSPIval)
    print('ToSPI : ',ToSPI)
  FromSPI = MySPI_FPGA.xfer2(ToSPI)
  sender.FromSPIval=hexltoint(FromSPI)
  sleep(5)

s.close()


