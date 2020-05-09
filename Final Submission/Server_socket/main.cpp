#include <cstdio>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <fcntl.h>      //Flags
#include <netinet/ip.h> //imports the socket stuff+ IP functions
#include <sys/wait.h>
#include <pthread.h>

class process
{
public:
    char Process_name[30];
    char status[15];
    char p_id[20];
    time_t starttime;
    time_t endtime, elapsedTime;

    char StartTimeStr[100];
};

class client
{
public:
    //char clientName[30];
    char status[15];
    char ID[20];
    int sockFD;
    int pipeReadFD;
    int pipeWriteFD;
    int pID;
};

process list[50];
int listSize_G;

client clientList[50];
int clientListSize_G;

int serverReadPipe[2], serverWritePipe[2], clientCombinedPipe[2], sock_fd;

void killProcess(process processList[], int p_ID, int status);

void *serverInput(void *parameter); //thread

void *clientSockReader(void *sockFDParameter); //thread

void *clientPipeReader(void *clientHandlerPipe); //thread

void add(int *storeValue, int addValue)
{
    *storeValue = *storeValue + addValue;
}

void sub(int *storeValue, int subValue, int *subCount)
{
    if (*subCount == 0)
    {
        *storeValue = subValue;
        *subCount = 1;
    }
    else
    {
        *storeValue = *storeValue - subValue;
    }
}

void mul(int *storeValue, int mulValue)
{
    *storeValue = *storeValue * mulValue;
}

void div(int *storeValue, int divValue, int *divCount)
{
    if (*divCount == 0)
    {
        *storeValue = divValue;
        *divCount = 1;
    }
    else
    {
        if (divValue != 0) //div by 0 check
        {
            *storeValue = (*storeValue) / divValue;
        }
    }
}
void termHandler(int signalNumber)
{
    if (signalNumber == SIGCHLD)
    {
        int process_ID = waitpid(-1, NULL, WNOHANG);
        while (process_ID > 0)
        {
            killProcess(list, process_ID, 0);
            process_ID = waitpid(-1, NULL, WNOHANG);
        }
    }
}

void ClientHandlerDeathHandler(int signalNumber)
{
    if (signalNumber == SIGCHLD)
    {
        int process_ID = waitpid(-1, NULL, WNOHANG);
        while (process_ID > 0)
        {
            for (int i = 0; i < clientListSize_G; i++)
            {
                if (clientList[i].pID == process_ID)
                {
                    strcpy(clientList[i].status, "Dead\0");
                }
            }

            process_ID = waitpid(-1, NULL, WNOHANG);
        }
    }
}

void getTime(char bufferTime[])
{
    int returnValue, hours, minutes, seconds;
    time_t currentTime;
    struct tm *localTime;
    time(&currentTime);
    hours = localTime->tm_hour;
    minutes = localTime->tm_min;
    seconds = localTime->tm_sec;
    returnValue = sprintf(bufferTime, " %d : %d : %d ", hours, minutes, seconds);
    bufferTime[returnValue] = '\0';
}

int runProgram(char *program, process processList[], int *sockFD) // Maintains the list and Runs a Program
{
    signal(SIGCHLD, termHandler);
    int returnValue, fdPipe[2];
    char buffer[500], *process_ID;
    time_t currentTime;

    returnValue = sprintf(buffer, "%s", program);
    buffer[returnValue] = '\0';
    strcpy(processList[listSize_G].Process_name, buffer);
    strcpy(processList[listSize_G].status, "Active\0");
    processList[listSize_G].starttime = time(NULL);
    processList[listSize_G].endtime = 0;
    pipe2(fdPipe, O_CLOEXEC);
    int run_pid = fork();

    if (run_pid > 0)
    {
        close(fdPipe[1]);
        returnValue = read(fdPipe[0], buffer, 15); //get the PID from the child process
        process_ID = strtok(buffer, "X");          //using STRTOK to add a null at the end of the string without knowing it's count/length
        strcpy(processList[listSize_G].p_id, process_ID);
        pid_t execPid = (pid_t)atoi(process_ID);
        returnValue = read(fdPipe[0], buffer, 10);
        if (returnValue != 0) //EXEC Failed
        {
            kill(execPid, SIGTERM);
            write(*sockFD, "CLIENT : EXEC FAILED\n", 21);
            return 0;
        }
        else
        {
            write(*sockFD, "CLIENT : EXEC SUCCESSFUL\n", 25);
            return 1;
        }
    }
    else //if (run_pid == 0)
    {
        close(fdPipe[0]);
        pid_t p_ID = getpid();
        returnValue = sprintf(buffer, "%d", p_ID);
        buffer[returnValue] = 'X'; //no garbage here
        buffer[returnValue + 1] = '\0';
        int length = strlen(buffer);
        write(fdPipe[1], buffer, length);
        int exec_return = execlp(program, program, NULL);
        if (exec_return == -1)
        {
            write(fdPipe[1], "Exec Filed\0", 11);
            perror("Exec");
        }
        return -1;
    }
}

int listShow(process processList[], char listOutput[])
{

    int currentIndex = 0;

    currentIndex += sprintf(&listOutput[currentIndex], "\n%s\t\t%s\t%s\t%s\t%s\t%s\n\n", "PROCESS NAME", "PID", "START TIME", "END TIME", "TIME ELAPSED", "STATUS");
    for (int i = 0; i < listSize_G; i++)
    {
        struct tm thisTime, thisTime2;
        time_t startTime = processList[i].starttime;
        time_t endTime = processList[i].endtime;
        localtime_r(&startTime, &thisTime);
        localtime_r(&endTime, &thisTime2);

        if (strcasecmp(processList[i].Process_name, "gnome-calculator") != 0)
        {
            currentIndex += sprintf(&listOutput[currentIndex], "%s\t\t\t", processList[i].Process_name);
            currentIndex += sprintf(&listOutput[currentIndex], "%s\t", processList[i].p_id);
            currentIndex += sprintf(&listOutput[currentIndex], "%d:%d:%d\t\t", thisTime.tm_hour, thisTime.tm_min, thisTime.tm_sec);
            currentIndex += sprintf(&listOutput[currentIndex], "%d:%d:%d\t\t", thisTime2.tm_hour, thisTime2.tm_min, thisTime2.tm_sec);
            currentIndex += sprintf(&listOutput[currentIndex], "%lu s\t\t", processList[i].elapsedTime);
            currentIndex += sprintf(&listOutput[currentIndex], "%s\n", processList[i].status);
        }
        else
        {
            currentIndex += sprintf(&listOutput[currentIndex], "%s\t", processList[i].Process_name);
            currentIndex += sprintf(&listOutput[currentIndex], "%s\t", processList[i].p_id);
            currentIndex += sprintf(&listOutput[currentIndex], "%d:%d:%d\t\t", thisTime.tm_hour, thisTime.tm_min, thisTime.tm_sec);
            currentIndex += sprintf(&listOutput[currentIndex], "%d:%d:%d\t\t", thisTime2.tm_hour, thisTime2.tm_min, thisTime2.tm_sec);
            currentIndex += sprintf(&listOutput[currentIndex], "%lu s\t\t", processList[i].elapsedTime);
            currentIndex += sprintf(&listOutput[currentIndex], "%s\n", processList[i].status);
        }
    }
    return currentIndex;
}

void killProcess(process processList[], int p_ID, int status) //issue 1 : if firefox already open and you kill it, wont work
{                                                             //issue 2 : if multiple gedit's => open and you kill the bottom ones nothing is going to happen, if you kill the top one all of them are going to get killed
                                                              //status =1 process is alive kill it, status=0 process is already dead (killed externally) change the status
    int returnValue;
    char process_ID[30];
    returnValue = sprintf(process_ID, "%d", p_ID);
    process_ID[returnValue] = '\0';
    for (int i = 0; i < listSize_G; i++)
    {
        if (strcasecmp(processList[i].p_id, process_ID) == 0)
        {
            pid_t this_pid = (pid_t)p_ID;
            if (status == 1)
            {
                kill(this_pid, SIGKILL);
                strcpy(processList[i].status, "Dead\0");
                processList[i].endtime = time(NULL);
                processList[i].elapsedTime = processList[i].endtime - processList[i].starttime;
            }
            else
            {
                strcpy(processList[i].status, "UnKillable\0");
                processList[i].endtime = time(NULL);
                processList[i].elapsedTime = processList[i].endtime - processList[i].starttime;
            }

            break;
        }
    }
}

int socketInitialization()
{
    char buffer[100];
    struct sockaddr_in server;
    int sock, returnValue, length, portNumber;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("SERVER : Socket Error ");
        exit(0);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;                           //socket is bound to all open ports??
    server.sin_port = 0;                                           // 0 means bind the socket to any available port
    returnValue = bind(sock, (sockaddr *)&server, sizeof(server)); //bind the process to a port
    if (returnValue < 0)
    {
        perror("SERVER : Bind Failed ");
        exit(0);
    }
    //port number assigned to this process is stored in server.port.. print it.
    length = sizeof(server);
    returnValue = getsockname(sock, (sockaddr *)&server, (socklen_t *)&(length));
    if (returnValue < 0)
    {
        perror("SERVER : Sock_name Error");
        exit(0);
    }
    portNumber = ntohs(server.sin_port);
    write(STDOUT_FILENO, "PORT NUMBER :\t", 14);
    returnValue = sprintf(buffer, "%d", portNumber);
    buffer[returnValue] = '\0';
    write(STDOUT_FILENO, buffer, returnValue);
    write(STDOUT_FILENO, "\n", 1);
    listen(sock, 10); // 10== queue size
    return sock;
}

int clientOperandSelection(char *strToken, int *calculation, int *divCount, int *subCount, int *sockFD)
{
    int operand;
    if (strlen(strToken) > 3)
    { //Exit needs null at the end if I want to use strcasecmp
        strToken[4] = '\0';
    }

    if (strcasecmp(strToken, "add") == 0)
    {
        operand = 1;
        *calculation = 0;
    }
    else if (strcasecmp(strToken, "sub") == 0)
    {
        operand = 2;
        *calculation = 0;
        *subCount = 0;
    }
    else if (strcasecmp(strToken, "mul") == 0)
    {
        operand = 3;
        *calculation = 1;
    }
    else if (strcasecmp(strToken, "div") == 0)
    {
        operand = 4;
        *calculation = 1;
        *divCount = 0;
    }
    else if (strcasecmp(strToken, "run") == 0)
    {
        operand = 5;
    }
    else if (strcasecmp(strToken, "list") == 0)
    {
        operand = 6;
    }
    else if (strcasecmp(strToken, "kill") == 0)
    {
        operand = 7;
    }
    else if (strcasecmp(strToken, "prin") == 0) //prin written on purpose, because im putting Null on position 4 of the strtoken
    {
        operand = 8;
    }
    else if (strcasecmp(strToken, "Exit") == 0)
    {
        //kill all the processes
        for (int i = 0; i < listSize_G; ++i)
        {
            int pid_int = atoi(list[i].p_id);
            pid_t this_pid = (pid_t)pid_int;
            kill(this_pid, SIGKILL);
        }
        close(*sockFD);
        write(STDOUT_FILENO, "SERVER : CONNECTION ABORTED\n", 28);
        exit(0);
    }
    else
    {
        *calculation = 0;
        write(STDOUT_FILENO, "Client Operand Selection : Wrong Command try again\n", 51);
        operand = -1;
    }
    return operand;
}
void populateClientList(int pID, int pipeReadFD, int pipeWriteFD)
{
    int returnValue;
    char buffer[20];
    returnValue = sprintf(buffer, "%s%d", "Client", clientListSize_G + 1);
    buffer[returnValue] = '\0';
    strcpy(clientList[clientListSize_G].ID, buffer);
    clientList[clientListSize_G].pID = pID;
    //clientList[clientListSize_G].sockFD = sockFD;
    clientList[clientListSize_G].pipeReadFD = pipeReadFD;
    clientList[clientListSize_G].pipeWriteFD = pipeWriteFD;
    strcpy(clientList[clientListSize_G].status, "Active\0");
    clientListSize_G++;
}

void printClientList(client clientList_local[], int clientListSize_local)
{
    char listOutput[500];
    int currentIndex = 0;
    currentIndex += sprintf(&listOutput[currentIndex], "\n%s\t\t%s\t%s\t%s\t%s\n\n", "ID", "STATUS", "PID", "Read", "Write");
    for (int i = 0; i < clientListSize_local; i++)
    {
        currentIndex += sprintf(&listOutput[currentIndex], "%s\t\t", clientList_local[i].ID);
        currentIndex += sprintf(&listOutput[currentIndex], "%s\t", clientList_local[i].status);
        currentIndex += sprintf(&listOutput[currentIndex], "%d\t", clientList_local[i].pID);
        currentIndex += sprintf(&listOutput[currentIndex], "%d\t", clientList_local[i].pipeReadFD);
        currentIndex += sprintf(&listOutput[currentIndex], "%d\n", clientList_local[i].pipeWriteFD);
    }
    write(STDOUT_FILENO, listOutput, currentIndex);
}

int main()
{
    //struct sockaddr_in server;
    bool infiniteLoop = false;
    int sock;
    int p_id = 3, inputThreadReturn; // p_id value assigned so that parent code runs before the fork
    pthread_t inputThread, clientSockHandler, clientPipeHandler;
    listSize_G = 0;
    clientListSize_G = 0;
    int runCount = 0;
    sock = socketInitialization(); //Establishing a connection

    write(STDOUT_FILENO, "Waiting for Connection Request\n", 31);
    bool clientHandlerThreadCreated = false;
    while (!infiniteLoop)
    {
        if (p_id > 0) //parent
        {
            //close(sock_fd);    //if I close sock_fd serverInput starts reading from the screen when I havent written anything there.
            sock_fd = accept(sock, 0, 0);
            write(STDOUT_FILENO, "SERVER : CONNECTION ESTABLISHED\n", 32);
            if (sock_fd < 0)
            {
                perror("SERVER : Accept Error");
                exit(0);
            }
            //Enter this client into the Client List
            pipe2(serverReadPipe, O_CLOEXEC);
            pipe2(serverWritePipe, O_CLOEXEC);
            p_id = fork();
            if (p_id > 0)
            {
                //close(serverWritePipe[0]);
                //close(serverReadPipe[1]);
                signal(SIGCHLD, ClientHandlerDeathHandler);
                populateClientList(p_id, serverReadPipe[0], serverWritePipe[1]);
                if (runCount < 1) //I only want this thread to get activated once not on every new client.
                {
                    inputThreadReturn = pthread_create(&inputThread, NULL, serverInput, (void *)NULL);
                    if (inputThreadReturn != 0)
                    {
                        perror("Server : Thread Create Error");
                    }
                    runCount++;
                    //close(sock_fd);
                }
                close(sock_fd);
            }
        }
        else if (p_id == 0) //child process
        {
            //close(serverWritePipe[1]);
            //close(serverReadPipe[0]);
            clientCombinedPipe[0] = serverWritePipe[0];
            clientCombinedPipe[1] = serverReadPipe[1];

            if (clientHandlerThreadCreated == false)
            {
                write(STDOUT_FILENO, "Client Handler Thread Created\n", 30);
                //pthread_t clientSockHandler;
                //pthread_t clientPipeHandler;
                pthread_create(&clientSockHandler, NULL, clientSockReader, (void *)NULL);
                pthread_create(&clientPipeHandler, NULL, clientPipeReader, (void *)NULL);
                clientHandlerThreadCreated = true;
            }
        }
    }
    pthread_join(inputThread, NULL);
    pthread_join(clientSockHandler, NULL);
    pthread_join(clientPipeHandler, NULL);
}

int serverOperandSelection(char *inputString)
{
    if (strcasecmp(inputString, "list") == 0)
    {
        return 1;
    }
    else if (strcasecmp(inputString, "printAll") == 0)
    {
        return 2;
    }
    else if (strcasecmp(inputString, "print") == 0)
    {
        return 3;
    }
    else if (strcasecmp(inputString, "ClientList") == 0)
    {
        return 4;
    }
    else
    {
        return -1;
    }
}

void printList_All()
{
    char buffer[500];
    int returnValue;
    for (int i = 0; i < clientListSize_G; i++)
    {
        if (strcasecmp(clientList[i].status, "Dead") != 0)
        {
            write(clientList[i].pipeWriteFD, "list", 4);
            returnValue = read(clientList[i].pipeReadFD, buffer, 500);
            write(STDOUT_FILENO, clientList[i].ID, strlen(clientList[i].ID));
            write(STDOUT_FILENO, "\n", 1);
            write(STDOUT_FILENO, buffer, returnValue);
        }
    }
}

void printList_Specified(char *clientID)
{
    char buffer[500];
    int returnValue;
    bool found = false;
    for (int i = 0; i < clientListSize_G; i++)
    {
        if (strcasecmp(clientList[i].ID, clientID) == 0 && strcasecmp(clientList[i].status, "Dead") != 0)
        {
            write(clientList[i].pipeWriteFD, "list", 4);
            returnValue = read(clientList[i].pipeReadFD, buffer, 500);
            write(STDOUT_FILENO, clientList[i].ID, strlen(clientList[i].ID));
            write(STDOUT_FILENO, "\n", 1);
            write(STDOUT_FILENO, buffer, returnValue);
            found = true;
            break;
        }
    }
    if (found == false)
    {
        write(STDOUT_FILENO, "CLIENT NOT FOUND\n", 17);
    }
}

void printMessage_ALL(char printBuffer[500])
{
    char buffer[6];
    for (int i = 0; i < clientListSize_G; i++)
    {
        if (strcasecmp(clientList[i].status, "Dead") != 0)
        {
            write(clientList[i].pipeWriteFD, "print", 5);
            read(clientList[i].pipeReadFD, buffer, 5);
            buffer[5] = '\0';
            if (strcasecmp(buffer, "print") == 0) //recieved acknowledgement
            {
                write(clientList[i].pipeWriteFD, printBuffer, strlen(printBuffer));
            }
        }
    }
}

void printMessage_Specified(char *clientID, char printBuffer[500])
{
    char buffer[6];
    bool found = false;
    for (int i = 0; i < clientListSize_G; i++)
    {
        if (strcasecmp(clientList[i].ID, clientID) == 0 && strcasecmp(clientList[i].status, "Dead") != 0)
        {
            write(clientList[i].pipeWriteFD, "print", 5);
            read(clientList[i].pipeReadFD, buffer, 5);
            buffer[5] = '\0';
            if (strcasecmp(buffer, "print") == 0) //recieved acknowledgement
            {
                write(clientList[i].pipeWriteFD, printBuffer, strlen(printBuffer));
            }
            found = true;
            break;
        }
    }
    if (found == false)
    {
        write(STDOUT_FILENO, "CLIENT NOT FOUND\n", 17);
    }
}

void *serverInput(void *parameter)
{
    char inputBuffer[1000];
    int returnValue, operand;
    char *strTokReturn;
    write(STDOUT_FILENO, "Server Is Reading\n", 18);
    while (true)
    {
        returnValue = read(STDIN_FILENO, inputBuffer, 1000);

        if (returnValue > 3)
        {
            inputBuffer[returnValue - 1] = '\0';
            strTokReturn = strtok(inputBuffer, " ");
            operand = serverOperandSelection(strTokReturn);
            if (operand == -1)
            {
                strTokReturn = NULL;
                write(STDOUT_FILENO, "SERVER : Invalid Input\n", 23);
            }
            else
            {
                strTokReturn = strtok(NULL, " ");
                if (strTokReturn == NULL)
                { //operand to ALL  Just Print or List
                    if (operand == 1)
                    {
                        printList_All();
                    }
                    else if (operand == 4)
                    {
                        printClientList(clientList, clientListSize_G);
                    }
                }
                else
                {
                    if (operand == 1)
                    {
                        printList_Specified(strTokReturn);
                    }
                    else if (operand == 2)
                    {
                        char printBuffer[500];
                        int printBufferSize = 0;
                        while (strTokReturn != NULL)
                        {
                            printBufferSize += sprintf(&printBuffer[printBufferSize], " %s", strTokReturn);
                            strTokReturn = strtok(NULL, " ");
                        }
                        printBuffer[printBufferSize] = '\0';

                        printMessage_ALL(printBuffer);
                    }
                    else if (operand == 3)
                    {
                        char ID[10];
                        char printBuffer[500];
                        int printBufferSize = 0;
                        returnValue = sprintf(ID, "%s", strTokReturn);
                        ID[returnValue] = '\0';
                        strTokReturn = strtok(NULL, " ");
                        while (strTokReturn != NULL)
                        {
                            printBufferSize += sprintf(&printBuffer[printBufferSize], " %s", strTokReturn);
                            strTokReturn = strtok(NULL, " ");
                        }
                        printBuffer[printBufferSize] = '\0';
                        printMessage_Specified(ID, printBuffer);
                    }
                    else if (operand == 4)
                    {
                        printClientList(clientList, clientListSize_G);
                    }
                }
            }
        }
    }
    write(STDOUT_FILENO, "SERVER INPUT : THREAD DESTROYED\n", 32);
}

void *clientSockReader(void *parameter)
{
    char buffer[50], *strtokReturn, output[500], printBuffer[500], inputChar[10];
    int returnValue, operand, calculation, divCount, subCount, tokenValue, printBufferSize;
    char input_c[500];
    int bufferSize;
    while (true)
    {
        //change start
        bufferSize = 0;
        while (true)
        {

            returnValue = read(sock_fd, inputChar, 1); //read the input from the pipe and store it in input_c
            if (returnValue <= 0)
            {
                perror("SERVER : Sock Read Error");
                exit(0);
            }
            else
            {
                if (strcasecmp(&inputChar[0], ";") == 0)
                { //delimiter found
                    write(STDOUT_FILENO, "Server : Semi Colon Found\n", 26);
                    break;
                }
                else
                {
                    input_c[bufferSize] = inputChar[0];
                    bufferSize += returnValue;
                }
            }
        }
        input_c[bufferSize] = '\0';

        //change end
        strtokReturn = strtok(input_c, " "); //delimit the input
        if (strtokReturn != NULL)            // \n only
        {
            returnValue = sprintf(output, "%s", strtokReturn);
            operand = clientOperandSelection(strtokReturn, &calculation, &divCount, &subCount, &sock_fd); //Operand Selection
            if (operand != 6)
            {
                strtokReturn = strtok(NULL, " ");
            }
        }
        else // \n only
        {
            operand = -1;
        }
        printBufferSize = 0;

        while (strtokReturn != NULL)
        {
            if (operand == -1)
            {
                calculation = 0;
                break;
            }
            //REGEX CHECK FOR A CHARACTER BEFORE AN INTEGER
            //regexec()
            if (operand <= 4 || operand == 7)
            {
                for (int i = 0; i < strlen(strtokReturn); i++)
                {
                    if (isdigit(strtokReturn[i]) == 0)
                    {
                        write(sock_fd, "CLIENT : Cant enter Characters for this Operand\n", 48);
                        operand = -1;
                        break;
                    }
                }

                sscanf(strtokReturn, "%d", &tokenValue);
                strtokReturn = strtok(NULL, " ");
            }
            if (operand == 1) //addition.
            {
                add(&calculation, tokenValue);
            }
            else if (operand == 2) //subtract
            {
                sub(&calculation, tokenValue, &subCount);
            }
            else if (operand == 3) //multiply
            {
                mul(&calculation, tokenValue);
            }
            else if (operand == 4) //division
            {
                div(&calculation, tokenValue, &divCount);
            }
            else if (operand == 5) //run
            {
                operand = 0; //set it to 0 so it doesnt remain 5 in which case it wont allow me to take any further inputs
                char *file_path = strtokReturn;
                returnValue = runProgram(file_path, list, &sock_fd);
                if (returnValue == 1)
                {
                    listSize_G++;
                }
                strtokReturn = NULL;
            }
            else if (operand == 6) //list show
            {
                char listOutput[1000];
                int outputSize = listShow(list, listOutput /*, list_count*/);
                write(sock_fd, listOutput, outputSize);
                strtokReturn = NULL;
            }
            else if (operand == 7) //kill
            {
                int p_ID = tokenValue;
                killProcess(list, p_ID, /*list_count,*/ 1);
                strtokReturn = NULL;
            }
            else if (operand == 8) //Print to Server
            {
                printBufferSize += sprintf(&printBuffer[printBufferSize], " %s", strtokReturn);
                strtokReturn = strtok(NULL, " ");
                if (strtokReturn == NULL)
                {
                    printBufferSize += sprintf(&printBuffer[printBufferSize], "%s\n", ".");
                    write(STDOUT_FILENO, printBuffer, printBufferSize);
                }
            }
        }
        if (operand <= 4 && operand > 0)
        {
            returnValue = sprintf(output, "%d", calculation);
            output[returnValue] = '\n';
            output[returnValue + 1] = '\0';
            write(sock_fd, output, returnValue + 2);
        }
        else if (operand != 6)
        {
            write(sock_fd, "\n", 1);
        }
    }
}

void *clientPipeReader(void *clientHandlerPipe)
{
    char buffer[25], listOutput[500], printBuffer[500];
    int returnValue;
    while (true)
    {
        returnValue = read(clientCombinedPipe[0], buffer, 25);
        buffer[returnValue] = '\0';
        if (strcasecmp(buffer, "list") == 0)
        {
            if (listSize_G == 0)
            {
                write(clientCombinedPipe[1], "Client List is Empty\n", 21);
            }
            else
            {
                int listOutputSize = listShow(list, listOutput);
                write(clientCombinedPipe[1], listOutput, listOutputSize);
            }
        }
        else if (strcasecmp(buffer, "print") == 0)
        {
            write(clientCombinedPipe[1], "print", 5);
            returnValue = read(clientCombinedPipe[0], printBuffer, 500);
            printBuffer[returnValue] = '\n';
            returnValue++;
            write(sock_fd, printBuffer, returnValue);
            //Call print
        }
        else if (strcasecmp(buffer, "exit") == 0)
        {
           //exit
        }
    }
}