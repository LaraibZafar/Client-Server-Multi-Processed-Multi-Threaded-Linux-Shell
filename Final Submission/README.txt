NAME : Muhammad Laraib uz Zafar
ERP : 14960

To Compile The Server -> Go to the cpp file's directory-> g++ -pthread main.cpp
To Compile The Client -> Go to the cpp file's directory-> g++ -pthread main.cpp
To Run The Server type -> ./a.out
To Run The Client type -> ./a.out IP PortNumber

Note : 
If both the Server and Client are being run on the same machine IP => localhost
If the Server and Client are being run on seperate machines IP => get the LOCAL IP of your Server Machine from Internet Settings

Valid Commands (CLIENT) : 

1- Add <List of digits seperated by space> => Add 12 14..
2- Sub <List of digits seperated by space> => Sub 12 14..
3- Div <List of digits seperated by space> => Div 100 5..
4- Mul <List of digits seperated by space> => Mul 12 14..
5- Run <Application Name> => Run gnome-calculator
6- List
7- Kill <Process ID> => Kill 12901
8- Print <Message> => Print This On The Server
9- Exit

Valid Commands (Server) :

1- ClientList
2- List
3- List <Specific ClientID> => List Client1
4- PrintAll <Message> => PrintAll Print This On Every Client's Screen
5- Print <Specific ClientID> <Message> => Print Client1 Print This On The Client's Screen

*Every Command has to be started from a New Line
 However If you put a SEMICOLON ( ; ) between the
 Commands you can put Multiple Commands on the Same
 Line
*Server can be terminated by CTRL + C which will
 automatically Terminate all the Clients Connected
 to it.