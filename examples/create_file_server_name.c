#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char **argv)
{
    FILE*  file = NULL;
    char  *node_name;
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;

    file = fopen("server_name.txt", "w+");

    if (file != NULL)
    {
//        gethostname(node_name, 256);

        getifaddrs (&ifap);
        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family==AF_INET) {
                sa = (struct sockaddr_in *) ifa->ifa_addr;
                node_name = inet_ntoa(sa->sin_addr);
                if (strcmp (ifa->ifa_name, "ib0") == 0)
                {
                    printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, node_name);
                    fputs(node_name ,file);
                    break;
                }
            }
        }

        fclose(file);
    }
    else
    {
        fprintf(stderr,"Error opening file server_name.txt");
    }

    return 0;
}
