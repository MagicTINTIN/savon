#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clientserver.h"

// HELP SECTION WITH OVERLOAD

void printHelp4(char const *progName, char const *errorDescription, char const *precision, char const *afterPrecision)
{
    fprintf(stderr, "%s%s%s", errorDescription, precision, afterPrecision);
    printf("%s -h|--help\
   \n\t-s,--server\
   \n\t-c|--client <serverIP>\
   \n\t[Optional]\
   \n\t[-p|--port <port>]\
   \n",
           progName);
    exit(0);
}

void printHelp2(char const *progName, char const *errorDescription)
{
    printHelp4(progName, errorDescription, "", "");
}

void printHelp1(char const *progName)
{
    printHelp2(progName, "");
}

void printHelp()
{
    printHelp1("savon");
}

int main(int argc, char const *argv[])
{
    int serv = -1;
    int port = DEFAULT_PORT;
    char distantAddress[256] = {0};
    for (unsigned int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
            printHelp1(argv[0]);

        // FILE SETTINGS
        else if (strcmp(argv[i], "--server") == 0 || strcmp(argv[i], "-s") == 0)
        {
            if (serv == 0)
                printHelp2(argv[0], "ERROR: already declared as Client (with -c/--client).\n");
            serv = 1;
        }
        else if (strcmp(argv[i], "--client") == 0 || strcmp(argv[i], "-c") == 0)
        {
            if (serv == 1)
                printHelp2(argv[0], "ERROR: already declared as Server (with -s/--server).\n");
            else if (argc - 1 < i + 1)
                printHelp2(argv[0], "ERROR: --client/-o takes 1 more param (string serverIP)\n");
            strncpy(distantAddress, argv[i + 1], 255);
            serv = 0;
            i += 1;
        }
        else if (strcmp(argv[i], "--port") == 0 || strcmp(argv[i], "-p") == 0)
        {
            if (argc - 1 < i + 1)
                printHelp2(argv[0], "ERROR: --port/-p takes 1 more param (int port)\n");
            port = atoi(argv[i + 1]);
            i += 1;
        }

        // THROW ERROR
        else
        {
            printHelp4(argv[0], "ERROR: UNKNOWN ARGUMENT \"", argv[i], "\"\n");
        }
    }

    // if server/client not specified
    if (serv == -1)
        printHelp2(argv[0], "ERROR: you need to specify wether you want a client or a server (with -c/--client or -s/--server).\n");

    if (serv == 1)
        mainServer(port);
    else if (serv == 0)
        mainClient(distantAddress, port);

    printf("Exiting SAVON...\n");
    return 0;
}
