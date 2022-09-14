#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <termios.h>

using namespace std;

void clean_stdin() {
    int stdin_copy = dup(STDIN_FILENO);
    tcdrain(stdin_copy);
    tcflush(stdin_copy, TCIFLUSH);
    close(stdin_copy);
}

const int N = 128, P = 256;
int flag_main = 0;

class Thread {
private:
    int c_socket, in = 0, out = 0, n = 0, flag_thread_1 = 0, flag_thread_2 = 0, flag_connect = 0;
    char Buf[N][N];
    char M[P];
    pthread_mutex_t mut;
    pthread_attr_t attr;
    pthread_t id_thread_1, id_thread_2, id_connect;
    sockaddr_in client_addr;

    void thread_1();
    void thread_2();
    void connect_thr();

    void socket_init();
    void socket_close();
    int sum();
    bool test_num();

    static void* thr_1(void* thisPtr);
    static void* thr_2(void* thisPtr);
    static void* Connection(void* thisPtr);

public:
    Thread();
    ~Thread();
};

void Thread::thread_1() {
    cout << "thread_1 start\n";
    while (flag_thread_1 == 0) {
        if (n != N) {
            n++;
            pthread_mutex_lock(&mut);
            cout << "thread_1:\nEnter number to process\n";
            cin >> M;
            while ((strlen(M) > 64 || !test_num()) && strcmp(M, "End")) {
                cout << "Error!\nEnter a string containing only numbers and no longer than 64 characters\n";
                cin >> M;
            }
            if (strcmp(M, "End") == 0) {
                flag_main = 1;
                pthread_cancel(id_thread_2);
                pthread_exit(NULL);
            }
            sort(M, M + strlen(M), greater<>());
            int size = strlen(M);
            for (int i = 0, j = 0; i < size; i++, j++) {
                if ((M[i] - '0') % 2 == 1) {
                    Buf[in][j] = M[i];
                }
                else {
                    Buf[in][j] = 'K';
                    Buf[in][j + 1] = 'B';
                    j++;
                }
            }
            pthread_mutex_unlock(&mut); in = (in + 1) % N;
            cout << "\tSort: " << M << endl;
            memset(M, 0, P);
        }
        sleep(1);
    }
    pthread_exit(NULL);
}

void Thread::thread_2() {
    cout << "thread_2 start\n";
    while (flag_thread_2 == 0) {
        char sndbuf[256];
        if (n != 0) {
            n--;
            pthread_mutex_lock(&mut);
            cout << "thread_2:\n\tData from thread_1: " << Buf[out] << "\n\tSum of digits: " << sum() << endl;

            int len = sprintf(sndbuf, "%d\t", sum());
            int sentcount = send(c_socket, sndbuf, len, 0);

            int reccount = recv(c_socket, NULL, 0, 0);
            if (reccount == 0) {
                flag_thread_1 = flag_thread_2 = 1;
                socket_close();
                socket_init();
                pthread_create(&id_connect, &attr, &Thread::Connection, this);
            }
            out = (out + 1) % N;
            pthread_mutex_unlock(&mut);
        }
        sleep(1);
    }
    pthread_exit(NULL);
}

void Thread::connect_thr() {
    while (flag_connect == 0) {
        int result = connect(c_socket, (struct sockaddr*)&client_addr, sizeof(client_addr));
        if (result == -1) {
            perror("connect");
            sleep(1);
        }
        else {
            cout << "Connection established\n";
            clean_stdin();
            pthread_create(&id_thread_1, &attr, &Thread::thr_1, this);
            pthread_create(&id_thread_2, &attr, &Thread::thr_2, this);
            flag_thread_1 = flag_thread_2 = 0;
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}

void* Thread::Connection(void* thisPtr) {
    ((Thread*)thisPtr)->connect_thr();
    return NULL;
}

void* Thread::thr_1(void* thisPtr) {
    ((Thread*)thisPtr)->thread_1();
    return NULL;
}

void* Thread::thr_2(void* thisPtr) {
    ((Thread*)thisPtr)->thread_2();
    return NULL;
}

void Thread::socket_init() {
    c_socket = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(c_socket, F_SETFL, O_NONBLOCK);
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(4000);
    client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

void Thread::socket_close() {
    shutdown(c_socket, 2);
    close(c_socket);
}

int Thread::sum() {
    int size = strlen(Buf[out]), x = 0;
    for (int i = 0; i < size; i++) {
        if (Buf[out][i] > '0' && Buf[out][i] <= '9') {
            x += (Buf[out][i] - '0');
        }
    }
    return x;
}
bool Thread::test_num() {
    int size = strlen(M);
    for (int i = 0; i < size; i++) {
        if (M[i] < '0' || M[i] > '9') {
            return 0;
        }
    }
    return 1;
}

Thread::~Thread() {
    pthread_cancel(id_thread_1);
    pthread_cancel(id_thread_2);
    pthread_cancel(id_connect);
    pthread_mutex_destroy(&mut);
    socket_close();
}

Thread::Thread() {
    pthread_attr_init(&attr);
    pthread_mutex_init(&mut, NULL);
    socket_init();
    pthread_create(&id_connect, &attr, &Thread::Connection, this);
}

int main() {
    Thread* em = new Thread();
    cout << "\nProgram_1:\nTo exit the program type 'End'(when there is a connection).\n\n";

    while (flag_main == 0);

    delete em;

    cout << "\nProgram_1 finished\n";

    return 0;
}
