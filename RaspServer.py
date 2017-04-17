import socket
import threading
import serial
import Queue


#Settings
HOST = ''
PORT = 8088
sendQ = Queue.Queue()
recvQ = Queue.Queue()

#Command list
remote_list = ['Rleft','Rrigh','Rforw','Rback', 'Rstop']
setting_list = [ 'St', 'Sl', 'Sr', 'Ss', 'Sf', 'So']

#Serial Setting
ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
ser.open()

#Socket Setting
soc = socket.socket()
soc.bind((HOST, PORT))
soc.listen(2)
conn, addr = soc.accept()
print "Connected by", addr
socket_connected = 1

#Arduino -> Raspberry
def fromArdu():
	while True:
		sendData_Rasp = ser.readline()	#read serial and store at sendData_Rasp
		if not sendData_Rasp:	#if there is no data, receive again
			continue
		else:		#if collectly recevied
			sendQ.put(sendData_Rasp)	#put into sendQueue

#Raspberry -> Arduino
def toArdu():
	while True:
		if not recvQ.empty():	#if Received queue is not empty
			comm = recvQ.get()	#store at command
			#fix this part
			if comm[:5] in remote_list:	#if command is in list
				print "Java -> Python -> Arduino : ", comm[:2]
				ser.writelines(comm[:2])	#send first two commands to Arduino
			elif comm[:2] in setting_list: #if command is in list
				print "Java -> Python -> Arduino : ", comm
				ser.writelines(comm)	#send first two commands to Arduino
			else:		#if received data is not valid
				continue	#ignore that data
	
#Raspberry -> App
def toApp():
	while True:
		if not sendQ.empty():	#if send queue is not empty
			sendData_App = sendQ.get()	#store at sendData_App
			isSent = conn.send(sendData_App)	#send and sotre result at isSent
			if not isSent:		#if send fail break while and close connection
				print "Not sent ", isSent
				break
			else:
				print "Arduino -> Python -> Java : ", sendData_App
	conn.colse()
	socket_connected = 0
	print "Soket closed"
	
#App -> Raspberry
def fromApp():
	while True:
		fApp_data = conn.recv(1024)	#Data from App
		if not fApp_data: 		#if lost connection, close socket
			break 
		fApp_data = fApp_data[2:]		#delete 0x00,0x04
		recvQ.put(fApp_data)	#put into receiveQueue
	conn.close()
	global socket_connected
	socket_connected = 0
	print "Socket closed!"


	
#Threading
threading._start_new_thread(fromApp,())
threading._start_new_thread(toApp,())
threading._start_new_thread(fromArdu,())
threading._start_new_thread(toArdu,())


#Main Function
while True:
	if not socket_connected:
		print "Wait for new connection"
		conn, addr = soc.accept()
		print "Connected by", addr
		socket_connected = 1
		threading._start_new_thread(fromApp,())
	else:
		continue