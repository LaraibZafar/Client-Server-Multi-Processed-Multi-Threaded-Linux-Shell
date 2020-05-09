#include <unistd.h>
#include <iostream>
#include <pthread.h>
//#include <sys/socket.h>//socket and the protocols
#include <netinet/ip.h> //imports the socket stuff+ IP functions
#include <netdb.h>      //imports the hostent structure
#include <string.h>     //for bcpy

void *socketRead(void *parameter);
void *socketWrite(void *parameter);
int sock_fd;
int main(int argc, char *argv[])
{
    pthread_t threadRead, threadWrite;
    int threadReadReturn, threadWriteReturn;
    struct sockaddr_in server;
    struct hostent *hosten_address;
    bool infiniteLoop = false;
    int read_return, connect_return;
    //write(STDOUT_FILENO, "Enter Data in the following Format  : Operand num1 num2 num3\n", 61);
    write(STDOUT_FILENO, "Operands : add sub mul div run list kill\n", 41);
    write(STDOUT_FILENO, "Calculation Command Format : Operand num1 num2 num3 ....\n", 57);
    write(STDOUT_FILENO, "Run Command Format : Run Document\n", 34);
    write(STDOUT_FILENO, "List Command Format : List\n", 20);
    write(STDOUT_FILENO, "Kill Command Format : Kill Process_ID\n", 38);
    write(STDOUT_FILENO, "Press Exit on a new line to exit\n", 33);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    //first argument : IPv4 communication channel    second argument : simple 2 way communication
    //third argument specified which protocol if a certain communication has multiple protocols
    if (sock_fd < 0)
    {
        perror("CLIENT : Socket Error ");
        exit(0);
    }
    //connecting the socket to whatever IP was provided
    server.sin_family = AF_INET; //ipv4 connection
    //convert the dotted IP and port number into hosten structure
    hosten_address = gethostbyname(argv[1]);
    //simply copies the IP to a hosten structure unless aliases are being used in which case some other operations have to be performed
    if (hosten_address == 0)
    {
        perror("CLIENT : unknown Host");
        exit(0);
    }
    bcopy(hosten_address->h_addr, &server.sin_addr, hosten_address->h_length);
    //store the hostent address in the server structure

    server.sin_port = htons(atoi(argv[2]));
    //convert the Port number to int and convert it into something understandable by the network and store it.
    //bind the server and the socket
    connect_return = connect(sock_fd, (struct sockaddr *)&server, sizeof(server));
    if (connect_return < 0)
    {
        perror("CLIENT : Connect Failed");
        exit(0);
    }
    write(STDOUT_FILENO, "CLIENT : CONNECTION ESTABLISHED\n", 32);
    threadReadReturn = pthread_create(&threadRead, NULL, socketRead, (void *)NULL);
    if (threadReadReturn != 0)
    {
        perror("CLIENT : Read Thread Creation Failed");
    }
    threadWriteReturn = pthread_create(&threadWrite, NULL, socketWrite, (void *)NULL);
    if (threadWriteReturn != 0)
    {
        perror("CLIENT : Write Thread Creation Failed");
    }
    pthread_join(threadRead, NULL);
    pthread_join(threadWrite, NULL);

}

void *socketRead(void *parameter)
{
    int returnValue;
    char inputRead[500];
    while (true)
    {
        returnValue = read(sock_fd, inputRead, 500);
        if (returnValue <= 0)
        {
            write(STDOUT_FILENO, "CLIENT : CONNECTION ABORTED WHILE READING\n", 42);
            exit(0);
        }
        write(STDOUT_FILENO, inputRead, returnValue);
    }
}

void *socketWrite(void *parameter)
{
    int returnValue;
    char inputWrite[500];
    while (true) //infinite Loop
    {
        returnValue = read(STDIN_FILENO, inputWrite, 500);
        inputWrite[returnValue - 1] = ';';
        returnValue = write(sock_fd, inputWrite, returnValue); //write the entire input to the pipe
        if (returnValue <= 0)
        {
            write(STDOUT_FILENO, "CLIENT : CONNECTION ABORTED WHILE WRITING\n", 42);
            exit(0);
        }
    }
}