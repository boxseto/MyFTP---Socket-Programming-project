# csci4430_asg1 - socket programming
## Practice basic socket programming using C
- this project uses Sockets to send and receieve data using FTP
- there are three commands for client:
- List (list file in server's local directory)
- put (upload a local file to server's local directory)
- get (downloac a server's local directory file to local directory of client)
## usage
- for server:
```
$ ./myftpserver <PORT>
```
- for Client:
```
$ ./myftpclient <SERVER_IP> <PORT> <LIST|GET|PUT> <FILE_TO_GET|FILE_TO_PUT>
```

