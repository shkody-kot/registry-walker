#include "main.h"
/*
//	Author: Kot
//	Dependencies: None
//	Build: 1.0.0
//	May 12, 2026
//
//	Inner workings of registry structures taken from Mimikatz, EZ Tools Registry Explorer
//	and https://binaryforay.blogspot.com/2015/01/registry-hive-basics.html.
//	Shockingly, not much has changed in the registry since 2015.
*/



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

static void help()
{
	printf("REGISTRY WALK\n");
	printf("To run: ./registry-walk\n");
	printf("The program will prompt for the path to a hive.\n");
	printf("\tCurrently no timestamps or types are returned by the walker, only the raw bytes of data, in hex\n");
	printf("\tIf you know that your data is a string, you can indicate it. The program will try its best to print it as such\n");
		
}

int main(int argc, char ** argv, char ** envp)
{
	if (argc > 1) { help(); return 1; }
	
	char * hive_path = calloc(256, sizeof(char));
	char * keypath;
	struct REGISTRY_HIVE * hive;
	int done = 0;
	int ok = 0;
	
	printf("REGISTRY WALK\n");
	printf("Please enter the path to the registry hive: ");
	hive_path = get_input(256);
	
	ok = load_hive(hive_path, &hive);
	if (!ok)
	{
		printf("[registry] Error opening hive. Exiting...\n");
		free(hive_path);
		free(hive);
		return 1;
	}
	
	printf("Please enter the full path of the registry key to fetch or q to quit.\n");
	printf("The first key is always called ROOT, not the name of the hive.\n");
	printf("EXAMPLE: ROOT\\ControlSet001\\Control\\TimeZoneInformation\\TimeZoneNameKey\n");
	while(!done)
	{
		keypath = calloc(256, sizeof(char));
		printf("[registry] key: ");
		keypath = get_input(256);
		
		if (strcmp(keypath, "q") == 0)
		{
			done = 1;
			break;
		}
		else
		{
			//create an output for the data
			unsigned char * data;
			int length;
			ok = registry_query_key(hive, keypath, &data, &length);
			if (!ok) { printf("[registry] key query failed"); }
			
			printf("Is this key a known string type? (y/n) ");
			int mode = getchar();
			char c;
			while ((c = getchar()) != '\n' && c != EOF);
			
			switch(mode)
			{
				case 'y':
				{
					char * char_data = calloc(length / 2, sizeof(char));
					int index = 0;
					for (int i = 0; i < length; i++) 
					{
						if (data[i])
						{
							char_data[index] = data[i]; 
							index++;  
						}
					}
					printf("[registry] %s found to be %s\n", keypath, char_data);
					break;
				}
				case 'n':
				{
					printf("[registry] %s found to be ", keypath);
					for (int i = 0; i < length; i++) { printf("%02x", data[i]); }
					printf("\n");
					break;
				}
			}
			
			
		}
		
		
		free(keypath);
	}
	
	ok = close_hive(hive);
	
	return 0;
}