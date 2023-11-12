Loic Dallaire
dallaireloic@gmail.com, ldallaire21@ubishops.ca
002311806

When executing, the server forks twice: once for the shell server and once for the file server, it then waits for 
the user to input 'q' and will shut down everything. Therefore it does not run in the background

For the threading, I basically have an array of size max_threads where everytime I want to start a thread I check if
I have a 0 in the array, if I do I start the thread and give it id of that index, and set the index at 1. When the tread 
finishes (loses connection) it sets the index to 0 and cleans up. The client can end the connection by sending QUIT the 
shell server or the file server, but does not have to do so, if it just disconnects the thread should also stop and clean
up. I did not have time to put a mutex on the array that controls the threads, but I have thought a lot about it and am
fairly certain that race conditions should not be an issue here.

The shell server and file server both work as expected and demanded by the assignment. It runs on a very simple 
protocol:
    - the server waits for input from the user (a command)
    - the server runs the command and crafts an appropriate response
    - the server sends that response back to the client
    - restart
so I did not use three way handshakes between client/server as it was not specified in the assignment

For the file access:
I have a struct containing three arrays:
    - one to keep track of opened file descriptors
    - one to keep track of reads on each file
    - one to keep track of writes on each file

These arrays are of size 128 as I assumed you would not open more than 128 files... reallocation would've been a bit complex.
The index of these arrays represents file descripters. So if the array that keeps track of fd at index 5 is 1, it means
fd5 is in use and is a valid file id. If writes at index 5 is 3 it means fd5 is being read by three different threads
and so on. Of course writes is binary as a file is only being writton to or not. Everything in that struct is controlled
by proper mutex and condition variable to assure we follow the read/write paradigm mentioned in the assignment.

For my tests, I wrote a simple client to interface with the server. The client starts up, asks for a port to connect
to and connects to localhost on that port. It then sends commands and recieves output. I ran multiple instances of this
client to test out that the file access and threading worked as expected, along with the shell server. My tests consisted
mainly of sending commands and making sure that the output was correct. I also tested the file access with multiple clients
with commands on the debug mode (with the -D switch). I also made use of the -v switch, in which if the function c_log()/2
is on mode VERBOSE and the switch is on I would print a bunch more information about everything that is going on. If you
wish to compile the client run `make client` and you will get a client executable.

