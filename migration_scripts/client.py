# Import socket module
import socket            
import sys

# Create a socket object
s = socket.socket()        
 
# Define the port on which you want to connect
port = 12345               
 
# connect to the server on local computer
s.connect((sys.argv[1], port))
data = "podman --runtime runc container restore --tcp-established --import-previous=pre-checkpoint_ws.tar.gz --import=checkpoint_ws.tar.gz"
s.send(data.encode())

print (s.recv(1024).decode())
# close the connection
s.close() 
