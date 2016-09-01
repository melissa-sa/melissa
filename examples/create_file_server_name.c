
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    FILE* file = NULL;
    char  node_name[256];

    file = fopen("server_name.txt", "w+");

    if (file != NULL)
    {
        gethostname(node_name, 256);

        fputs(node_name ,file);

        fclose(file);
    }
    else
    {
        fprintf(stderr,"Error opening file server_name.txt");
    }

    return 0;
}