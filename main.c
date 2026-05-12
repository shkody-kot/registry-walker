#include "main.h"

// generic get user input
char * get_input(size_t max_len)
{
    char * input = calloc(max_len, 1);
    if (!input) { return NULL; }

    if (!fgets(input, max_len, stdin)) 
	{
        free(input);
        return NULL;
    }

    // Trim newline
    input[strcspn(input, "\r\n")] = '\0';
    return input;
}

int main(int argc, char ** argv, char ** envp)
{
	
	return 0;
}