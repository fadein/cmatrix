#include <err.h>
#include <stdio.h>
#include <string.h>
#include <ccan/grab_file/grab_file.h>
#include <ccan/talloc/talloc.h> // For talloc_free()

int main(int argc, char *argv[])
{
        size_t len;
        char *file;

        file = grab_file(NULL, argv[1], &len);
        if (!file)
                err(1, "Could not read file %s", argv[1]);
        if (strlen(file) != len)
                printf("File contains NUL characters\n");
        else if (len == 0)
                printf("File contains nothing\n");
        else if (strchr(file, '\n'))
                printf("File contains multiple lines\n");
        else
                printf("File contains one line\n");
        talloc_free(file);

        return 0;
}
