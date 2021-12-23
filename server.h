#ifndef __SERVER_H__
#define __SERVER_H__

#define SERVER_PORT 4162
#define THREAD_NUM 10
#define STATIC_FILES_DIR "public"

#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <vector>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <memory>
#include <fstream>

using namespace std;

struct Context {
    int clientId;
    string method;
    string path;
    string requsetBody;
    int responseCode;
};

void sendFile(Context *ctx, string fileName);
void sendNotFound(Context *ctx);

#endif