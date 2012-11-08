#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static void usage(char *argv[])
{
    fprintf(stderr, 
            "Usage: %s <hostname> <port>   (client)\n"
            "       %s [ port ]            (server)\n", 
            argv[0], argv[0]);
    exit(EXIT_FAILURE);
}

const size_t MSGSIZE = 1024;

static void handle_error(bool condition, const char *msg)
{
    if (condition) {
        perror(msg);
        exit(1);
    }
}

static void handle_gai_error(int rc)
{
    if (rc) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rc));
        exit(1);
    }
}

static int get_socket(struct addrinfo *addr)
{
    int sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    handle_error(sock < 0, "creating socket");
    return sock;
}

static struct addrinfo *get_client_addrinfo(const char *host, const char *port)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *ai;
    int rc = getaddrinfo(host, port, &hints, &ai);
    handle_gai_error(rc);

    return ai;
}

static struct addrinfo *get_server_addrinfo(const char *port)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo *ai;
    int rc = getaddrinfo(nullptr, port, &hints, &ai);
    handle_gai_error(rc);

    return ai;
}

static int run_server(const char *port)
{
    
    int rc;

    addrinfo *ai = get_server_addrinfo(port);

    int listen_sock = get_socket(ai);

    int yes = 1;
    rc = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    handle_error(rc != 0, "setsockopt failed");

    rc = bind(listen_sock, ai->ai_addr, ai->ai_addrlen);
    handle_error(rc != 0, "bind failed");

    freeaddrinfo(ai);

    rc = listen(listen_sock, 10);
    handle_error(rc != 0, "listen failed");

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    rc = getsockname(listen_sock, (struct sockaddr *)&addr, &len);
    handle_error(rc != 0, "getsockname failed");

    printf("Server listening on port %d\n", ntohs(addr.sin_port));

    while (1) {
        char buf[MSGSIZE+1];

        int sock = accept(listen_sock, nullptr, 0);
        handle_error(sock < 0, "failed to accept");

        int nread = recv(sock, buf, MSGSIZE, 0);
        handle_error(nread < 0, "recv failed");

        printf("Received from client: %s\n", buf);

        sprintf(buf, "Reply from server %d!", getpid());
        rc = send(sock, buf, strlen(buf)+1, 0);

        close(sock);
    }

    close(listen_sock);

    return 0;
}

static int run_client(const char *server, const char *port)
{
    int rc;

    addrinfo *ai = get_client_addrinfo(server, port);

    int sock = get_socket(ai);

    rc = connect(sock, ai->ai_addr, ai->ai_addrlen);
    handle_error(rc < 0, "failed to connect");

    freeaddrinfo(ai);

    char buf[MSGSIZE+1];
    
    sprintf(buf, "Hello world from client %d!", getpid());
    rc = send(sock, buf, strlen(buf)+1, 0);
    handle_error(rc < 0, "send failed");

    rc = recv(sock, buf, MSGSIZE, 0);
    handle_error(rc < 0, "recv failed");
    printf("Received from server: %s\n", buf);

    close(sock);
    return 0;
}

int main(int argc, char *argv[])
{
    const char *server = NULL;
    const char *port = "0";

    if (argc > 1) {
        if (argc == 3) {
            server = argv[1];
            port = argv[2];
        } else if (argc == 2) {
            port = argv[1];
        } else {
            usage(argv);
        }
    }

    if (server != NULL) {
        return run_client(server, port);
    } else {
        return run_server(port);
    }
}
