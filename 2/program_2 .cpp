#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>

using namespace std;

int flag_connection_wait = 0, flag_accept_request = 0;

class Thread {

private:
    int listen_socket, server_socket;
    sockaddr_in listen_socket_addr;
    pthread_t id_accept_request, id_connection_wait;
    pthread_attr_t attr;

    static void* Connection(void* thisPtr);
    static void* Request(void* thisPtr);

    void connect_wait();
    void accept_request();
    void socket_init();
    void socket_close();

public:
    Thread();
    ~Thread();
};

void Thread::accept_request() {
    char recvbuf[128];
    int count = 0, sum = 0;
    while (flag_accept_request == 0) {
        memset(recvbuf, 0, 128);
        int reccount = recv(server_socket, recvbuf, sizeof(recvbuf), 0);
        if (reccount == -1) {
            perror("recv");
        }
        else if (reccount == 0) {
            shutdown(server_socket, 2);
            pthread_create(&id_connection_wait, &attr, &Thread::Connection, this);
            pthread_exit(NULL);
        }
        else {
            sscanf(recvbuf, "%d", &sum);
            cout << "Request received " << count << " : Number " << sum << endl;
            if (sum % 32 == 0 && sum > 99) {
                cout << "Data received\n";
            }
            else {
                cout << "Error\n";
            }
            count++;
        }
    }
    pthread_exit(NULL);
}

void Thread::connect_wait() {
    sockaddr_in server_socket_addr;
    socklen_t addr_len = (socklen_t)sizeof(server_socket_addr);

    while (flag_connection_wait == 0) {
        server_socket = accept(listen_socket, (struct sockaddr*)&server_socket_addr, &addr_len);
        if (server_socket == -1) {
            perror("accept");
            sleep(1);
        }
        else {
            cout << "Connection established\n";
            pthread_create(&id_accept_request, &attr, &Thread::Request, this);
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}

void* Thread::Connection(void* thisPtr) {
    ((Thread*)thisPtr)->connect_wait();
    return NULL;
}

void* Thread::Request(void* thisPtr) {
    ((Thread*)thisPtr)->accept_request();
    return NULL;

}

void Thread::socket_init() {
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(listen_socket, F_SETFL, O_NONBLOCK);

    listen_socket_addr.sin_family = AF_INET;
    listen_socket_addr.sin_port = htons(4000);
    listen_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int optval = 1;
    bind(listen_socket, (struct sockaddr*)&listen_socket_addr, sizeof(listen_socket_addr));
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    listen(listen_socket, SOMAXCONN);
}

void Thread::socket_close() {
    shutdown(server_socket, 2);
    close(listen_socket);
    close(server_socket);
}

Thread::~Thread() {
    pthread_cancel(id_accept_request);
    pthread_cancel(id_connection_wait);
    socket_close();
}

Thread::Thread() {
    pthread_attr_init(&attr);
    socket_init();
    pthread_create(&id_connection_wait, &attr, &Thread::Connection, this);
}

int main() {
    cout << "\nProgram_2 start:\nPress any key to exit.\n\n";

    Thread* em = new Thread();

    getchar();

    flag_connection_wait = flag_accept_request = 1;

    delete em;
    cout << "\nProgram_2 finished\n";

    return 0;
}
