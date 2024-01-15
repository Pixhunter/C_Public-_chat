# C_Public_chat
## Simple C server for public chat

#### How it works?<br>
The program is central server with simple SQL (using SQLite) that works with clients. 

#### How to Clients connect to server?
- from console using command like `netcat 127.0.0.0 7030` but you need to use server IP and port

#### What Server cans?
- Check valid username - only letters and numbers are available
- Register new users and save username / password to database
- Realised log in existing users
- Maintains user uniqueness and prevents two clients from logging in under the same username
- Limit the number of input attempts and disconnect user after it
- User can't see messages from other users while not log in
- All messages are signed with usernames


#### Schema of Server work
<img src="https://github.com/Pixhunter/C_Public_chat/blob/main/Untitled.jpg" width="500" height="300"/>
