**Hi!**

A client-server chat application written in C.

The application supports global and private conversations, file transfer, shell access, and user management.

**Features**:
- Global chat.
- Private conversations.
- File transfer.
- Shell access.
- Online users list.
- User directory.
- Client-server architecture.
- Server-side monitoring.

**Requirements**:  
- Linux/Unix-like operating system.  
- C compiler (gcc or clang).  

**Build**:  
make -f server.mk  
make -f client.mk  

**Running**:  
- Start the server:  
./run_server 9999  

- Then start the client:  
./run_client 127.0.0.1 9999

The chat currently supports the following commands on the client side:  
1) Start a private conversation.  
/private/username  
/private/ok/username  
Invite another user to a private conversation and accept the invitation.  

2) Send a file.  
/file/username  
/file/ok/username  
Request permission to send a file to another user and accept the request.  

3) Shell access.  
/shell/username  
/shell/ok/username  
Request shell access from another user and accept the request.  

4) Clear the screen.    
/clear  

6) Show online users.  
/online

7) Return to the global chat.  
/global

8) Browse the user directory.  
/browse

9) Exit the system.  
/exit


**Testing:**   
- Tested on an AWS EC2 instance.  
