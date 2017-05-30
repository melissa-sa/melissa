#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main (int argc, char **argv)
{
    FILE*  file = NULL;
    char   node_name[256];
    char  *node_addr;
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;

    file = fopen("master_name.txt", "w+");

    if (file != NULL)
    {
        gethostname(node_name, 256);

        getifaddrs (&ifap);
        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr->sa_family==AF_INET) {
                sa = (struct sockaddr_in *) ifa->ifa_addr;
                node_addr = inet_ntoa(sa->sin_addr);
                if (strcmp (ifa->ifa_name, "ib0") == 0)
                {
                    printf("Interface: %s\tAddress: %s\n", ifa->ifa_name, node_addr);
                    sprintf(node_name, "%s", node_addr);
                    break;
                }
            }
        }
        fputs(node_name ,file);
        fclose(file);
    }
    else
    {
        fprintf(stderr,"Error opening file master_name.txt");
    }

    return 0;
}
