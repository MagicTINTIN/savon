#pragma once

#define DEFAULT_PORT 5000 
#define MAXLINE 1000

int mainClient(const char *addr, int port);
int mainServer(int port);