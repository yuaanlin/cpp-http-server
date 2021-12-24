#include "server.h"

int sockfd;
struct sockaddr_in serverAddr;

void clearConsole() {
    for (int i = 0; i < 30; i++) {
        cout << endl;
    }
}

string getMimeType(string ext) {
    // if ext not start with . then add .
    if (ext[0] != '.') {
        ext = "." + ext;
    }

    if (ext == ".html") {
        return "text/html";
    } else if (ext == ".css") {
        return "text/css";
    } else if (ext == ".js") {
        return "application/javascript";
    } else if (ext == ".png") {
        return "image/png";
    } else if (ext == ".jpg") {
        return "image/jpeg";
    } else if (ext == ".gif") {
        return "image/gif";
    } else if (ext == ".ico") {
        return "image/x-icon";
    } else if (ext == ".txt") {
        return "text/plain";
    } else if (ext == ".pdf") {
        return "application/pdf";
    } else if (ext == ".zip") {
        return "application/zip";
    } else if (ext == ".mp3") {
        return "audio/mpeg";
    } else if (ext == ".mp4") {
        return "video/mp4";
    } else {
        return "text/plain";
    }
}

void logRequest(Context *ctx) {
    cout << to_string(ctx->responseCode) << " " << ctx->method << " "
         << ctx->path << endl;
}

void handlePostRequest(Context *ctx) {
    if (ctx->path == "/login") {
        if (ctx->requsetBody == "login=123&pass=456") {
            sendFile(ctx, "/login_success.html");
        } else {
            sendFile(ctx, "/login_failed.html");
        }
    } else {
        ctx->responseCode = 404;
        sendNotFound(ctx);
    }
}

void sendFile(Context *ctx, string fileName) {
    // if fileName not starting with slash, add slash
    if (fileName[0] != '/') {
        fileName = "/" + fileName;
    }

    // if fileName is empty then send index.html
    if (fileName.length() == 1) {
        fileName = "/index.html";
    }

    // get mimetype from file extension
    string ext = fileName.substr(fileName.find_last_of(".") + 1);
    string mime = getMimeType(ext);

    // read file
    char *sendbuf;
    FILE *requested_file;
    requested_file = fopen((STATIC_FILES_DIR + fileName).c_str(), "rb");

    // if file not found, send 404 page
    if (requested_file == NULL) {
        fclose(requested_file);
        sendNotFound(ctx);
        return;
    }

    fseek(requested_file, 0, SEEK_END);
    int fileLength = ftell(requested_file);
    rewind(requested_file);
    sendbuf = (char *)malloc(sizeof(char) * fileLength);
    size_t result = fread(sendbuf, 1, fileLength, requested_file);

    if (result > 0) {
        string h =
            "HTTP/1.1 200 OK\n"
            "Content-Length: " +
            to_string(fileLength) +
            "\n"
            "Content-Type: " +
            mime +
            "\n"
            "Content-Transfer-Encoding: binary\n"
            "Connection: Closed\n"
            "\n";
        send(ctx->clientId, h.c_str(), h.size(), 0);
        send(ctx->clientId, sendbuf, result, 0);
        ctx->responseCode = 200;
        logRequest(ctx);
        close(ctx->clientId);
    }

    fclose(requested_file);
}

void sendNotFound(Context *ctx) {
    ifstream file("404.html");
    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    file.close();

    string h =
        "HTTP/1.1 404 Not Found\n"
        "Content-Length: " +
        to_string(content.size()) + "\n" +
        "Content-Type: text/html\n"
        "Connection: Closed\n"
        "\n" +
        content;

    ctx->responseCode = 404;

    logRequest(ctx);

    send(ctx->clientId, h.c_str(), h.size(), 0);
}

/**
 * @brief A function which is always wating for new connection.
 * */
void waitNewConnection() {
    // This thread is running in an infinite loop
    // After an accepted connection is closed,
    // this thread will waiting for a new connection.
    while (1) {
        int clientId;
        struct sockaddr_in clientAddr;
        int iClientSize = sizeof(struct sockaddr_in);
        if ((clientId = accept(sockfd, (struct sockaddr *)&clientAddr,
                               (socklen_t *)&iClientSize)) == -1) {
            if (errno != 53) {
                cout << "accept() failed! code:" << errno << endl;
            }
            close(sockfd);
            break;
        }

        char buf[1024];
        int n = recv(clientId, buf, sizeof(buf), 0);
        if (n > 0) buf[n] = 0;

        string req(buf);

        string path = "";
        string method = "";
        string body = req.substr(req.find("\r\n\r\n") + 4, req.size());

        if (req.find("GET") != string::npos) {
            method = "GET";
            path = req.substr(req.find("GET") + 4,
                              req.find("HTTP") - 4 - req.find("GET"));
        } else if (req.find("POST") != string::npos) {
            method = "POST";
            path = req.substr(req.find("POST") + 5,
                              req.find("HTTP") - 5 - req.find("POST"));
        } else {
            cout << "unknow method" << endl;
            close(clientId);
            continue;
        }

        path.erase(path.find_last_not_of(" \n\r\t") + 1);

        // construct context
        Context ctx = {clientId, method, path, body};

        if (method == "GET") {
            sendFile(&ctx, ctx.path);
        }

        if (method == "POST") {
            handlePostRequest(&ctx);
        }

        close(clientId);
    }
}

void closeServer(int s) {
    cout << "Closing server ..." << endl;
    close(sockfd);
    exit(1);
}

int main() {
    clearConsole();

    // 创建一个socket：
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        cout << "socket() failed! code:" << errno << endl;
        return -1;
    }

    // 绑定：
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(serverAddr.sin_zero), 8);

    int port = SERVER_PORT;
    while (1) {
        serverAddr.sin_port = htons(port);
        if (::bind(sockfd, (struct sockaddr *)&serverAddr,
                   sizeof(serverAddr)) == -1) {
            cout << "port " << port << " cannot bind, trying next port ..."
                 << endl;
            port++;
            continue;
        }
        break;
    }

    // 侦听控制连接请求：
    if (listen(sockfd, 5) == -1) {
        printf("listen() failed! code:%d\n", errno);
        close(sockfd);
        return -1;
    }

    cout << endl << "Server is running on http://localhost:" << port << endl;
    cout << "Press Ctrl+c to quit!" << endl << endl;

// if starting server with mac, open website automatically
#if __APPLE__
    string url = "http://localhost:" + to_string(port);
    string cmd = "open -a \"Google Chrome\" " + url;
    system(cmd.c_str());
#endif

    // create waiting connection threads
    vector<int> iv;
    for (int i = 0; i < THREAD_NUM; i++) iv.push_back(i);
    vector<thread> thread_pool;
    thread_pool.reserve(THREAD_NUM);
    for (int &n : iv) {
        thread_pool.emplace_back(waitNewConnection);
    }

    // 捕捉服务关闭信号 (Ctrl+C)
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = closeServer;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    pause();
    return 0;
}
