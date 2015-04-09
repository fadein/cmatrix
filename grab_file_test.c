#include <ccan/grab_file/grab_file.h>
#include <ccan/talloc/talloc.h> // For talloc_free()
#include <err.h>
#include <stdio.h>
#include <string.h>

#define LINES 25
int main(int argc, char *argv[])
{
        size_t len;
		int i = 0;
        char *file, *p;
		char *lines[LINES];

		if (1 == strlen(argv[1]) && *argv[1] == '-')
			file = grab_file(NULL, NULL, &len);
		else
			file = grab_file(NULL, argv[1], &len);

		printf("Read %zd bytes from %s\n", len, argv[1]);

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

		p = strtok(file, "\n");
		while (p != NULL && i < LINES) {
			lines[i++] = p;
			p = strtok(NULL, "\n");
		}

		for (i = 0; i < LINES; ++i)
			printf("%s\n", lines[i]);

        talloc_free(file);

        return 0;
}
