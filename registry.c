#include "registry.h"

/*//HIVE STRUCTURE
	|	Hive Header (regf) 4096 bytes (1 block)
	|	- signature, file version, sequence numbers, root cell offset
	 -------------------------------
	| 	Hbin blocks (each hive contains one or more; these are the data)
	|	- multiple of 4096 bytes
	___________________________________
	| Key cell --> contains signature (nk or lk), timestamp for most recent edit, 
	|				cell index of the parent key cell, cell index of the subkey list cell, 
	|				cell index of security cell, and the name of the key
	| Value cell --> information about a key's value. contains signature (vk), the type, 
	|				the name of the value, and the cell index to the cell where the data isalnum
	| Subkey-list cell --> list of cell indices of key cells that belong to one parent key
	| Value-list cell --> list of cell indices of value cells that belong to one parent key
	| Security-descriptor cell --> i don't think i care about this

*/

//open a registry hive
//returns 1 on success, 0 otherwise. Outputs hive to provided address
int load_hive(char * hive_path, struct REGISTRY_HIVE ** hive)
{
	*hive = malloc(sizeof(struct REGISTRY_HIVE));
	(*hive)->name = calloc(MAX_LEN, sizeof(char));
	
	uint64_t filesize = 0;
	FILE * file = fopen(hive_path, "rb");
	if (!file) { perror("[registry] Couldn't open hive"); return 0; }
	
	//get size of file to calloc the hive in memory
	if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }

    // ftell returns long, check for negative before casting to uint64_t
    long sz = ftell(file);
    if (sz < 0) { fclose(file); return 0; }

    filesize = (uint64_t)sz;
	printf("[registry] filesize: %ld\n", filesize);
	//return cursor, as it were, to beginning of file
    if (fseek(file, 0, SEEK_SET) != 0) { fclose(file); return 0; }
	

	(*hive)->data = calloc(filesize, sizeof(uint8_t));
	
	//read the entire thing into memory because who cares about space efficiency
	int fd = fileno(file);
	read(fd, (*hive)->data, filesize);
	
	//get the offset of the root key of the registry
	//root cell offset lives at 0x24 and points to the top level key of a hive
	struct REG_KEY * rootkey = malloc(sizeof(struct REG_KEY));
	//initialize my key
	rootkey->name = calloc(MAX_LEN, sizeof(char));
	rootkey->valuekey = NULL;
	rootkey->subkey = NULL;
	rootkey->parent = NULL;
	
	rootkey->offset = 0x1020;
	(*hive)->root = rootkey;
	
	unsigned char * temp = calloc(MAX_LEN, sizeof(char));
	temp = (*hive)->data + rootkey->offset + NK_KEY_NAME;
	
	//the root key's name is always ROOT
	memcpy(rootkey->name, temp, 4);
	strcpy((*hive)->name, rootkey->name);
	
	fclose(file);
	return 1;
}

//close a registry hive
//delete all the REG_KEYs that I've created in there or valgrind will have me killed
//returns 1 on success, 0 otherwise
int close_hive(struct REGISTRY_HIVE * hive)
{
	registry_free(hive->root);
	free(hive->data);
	free(hive);
	return 1;
}

void registry_free(struct REG_KEY * root)
{
	struct REG_KEY * current = root;
	if (current->subkey != NULL) { registry_free(current->subkey); }
	if (current->nextkey != NULL) { registry_free(current->nextkey); }
	free(root);  
}

//query the value of a key
//returns 1 on success and 0 on failure
int registry_query_key(struct REGISTRY_HIVE * hive, char * target_key, unsigned char ** data, int * length)
{
	//walk down the registry to find the key we want
	struct REG_KEY * target = malloc(sizeof(struct REG_KEY));
	int level = 0;

	if (!registry_walk(hive, hive->root, target_key, level, &target)) { printf("[registry] walk failed\n"); }
			
	*data = calloc(target->data_size, sizeof(unsigned char));	
	memcpy(*data, target->data, target->data_size);
	*length = target->data_size;
	
	printf("[registry] found %s to be ", target->name);
	for (int i = 0; i < target->data_size; i++) { printf("%c", (*data)[i]); }
	printf("\n");
	
	return 1;
}

//make reg_key data type that stores the current path and all values for that key
//walk down the registry recursively until we find the target key
//if i want to optimize it later, do a binary search
int registry_walk(struct REGISTRY_HIVE * hive, struct REG_KEY * current_key, char * target_key, int level, struct REG_KEY ** out)
{
	//A subkey-list cell contains a list of cell indexes that refer to the subkey's key 
	//cells. Therefore, if you want to locate the key cell of a subkey that belongs 
	//to a particular key, you must first locate the cell containing the key's subkey 
	//list using the subkey-list cell index in the key's cell. Then, you locate each 
	//subkey cell using the list of cell indexes in the subkey-list cell. For each 
	//subkey cell, you check to see whether the subkey's name, which a key cell stores, 
	//matches the one you want to locate.
	if (current_key == NULL) { return 1; }
	struct REG_KEY * current = current_key;
	
	//find out how deep we'll have to go
	char delim = '\\';
	char null = '\0';
	int total_depth = 0;
	
	for (int index = 0; index < strlen(target_key); index++)
	{
		if (target_key[index] == delim) { total_depth++; }
	}
		
	//get the token of the target key at the needed depth (level) via strtok
	int position = 0;	//where we are in the target key vs where we need to be

	char * keypart = calloc(1024, sizeof(char));
	char * next_keypart = calloc(1024, sizeof(char));
	int next_count = 0;
	int count = 0;
	for (int index = 0; index < strlen(target_key) && position <= level; index++)
	{
		if (target_key[index] == delim) 
		{ 
			if (position != level)
			{
				//wipe keypart
				for (int i = 0; i < count; i++) { memcpy(keypart + i, &null, 1); }
				count = 0;
				position++;
			}
			else 
			{
				count = index;
				index = strlen(target_key);
			}
		}
		else 
		{ 
			strncpy(keypart + count, &target_key[index], 1); 
			count++;
		}
	}
	
	printf("[registry] seeking path part: %s at %d out of %d, currently on %s\n", keypart, position, total_depth, current->name);
	
	for (int index = count + 1; index < strlen(target_key); index++)
	{
		if (target_key[index] == delim) { index = strlen(target_key); }
		else
		{
			strncpy(next_keypart + next_count, &target_key[index], 1); 
			next_count++;
		}
	}
	//if we found the right subkey
	if (strcasecmp(current->name, keypart) == 0) 
	{
		if (!registry_scan_keys(hive, &current)) { printf("[registry] scan failed\n"); }
		
		printf("[registry] current key %s correct\n", current->name);
		if (level == total_depth) 
		{ 
			//printf("[registry] current key %s correct\n", current->name);
			memcpy(*out, current, sizeof(struct REG_KEY));
			return 1; 
		}
		else 
		{			
			//check all value keys first
			struct REG_KEY * tmp = current->valuekey;
			while (tmp != NULL && tmp->nextkey != NULL) 
			{
				printf("[registry] checking value  %s for %s\n", tmp->name, next_keypart);
				if (strncasecmp(tmp->name, next_keypart, strlen(next_keypart)) == 0) 
				{ 
					printf("[registry] current value key %s correct\n", tmp->name);
					memcpy(*out, tmp, sizeof(struct REG_KEY));
					return 1; 
				}
				tmp = tmp->nextkey;
			}
			
			position++;
			registry_walk(hive, current->subkey, target_key, position, out); 
		} 
	}
	//otherwise go to the next key 
	else { registry_walk(hive, current->nextkey, target_key, level, out); }
	
	//memcpy(*out, current, sizeof(struct REG_KEY));
	return 1;
}
//maps all values in a key
//returns pointer to the first value, and sets each subsequent as a pointer
//sets the provided root as the parent, keeps subkey as NULL
//determine the structure of a registry key as in headers and bullshit
int registry_scan_keys(struct REGISTRY_HIVE * hive, struct REG_KEY ** root)
{
	struct REG_KEY * current = *root;
	if (strcasecmp(current->name, hive->name) == 0) { current->parent = NULL; }

	//navigate to value cell's offset
	//navigate to value cell's data offset, and read the number of values.
	//if there is more than one value, set those values as their own keys. if there are zero, skip
	unsigned char * address = calloc(ADDRESS, sizeof(unsigned char));
	uint32_t int_address;
	
	//check if we've already scanned this key
	if (current->scanned) { return 1; }
	
	
	//get the number of values
	int num_values = 0;
	uint32_t list_address;
	int name_length;
	memcpy(&num_values, hive->data + current->offset + VALUE_COUNT, NUM_SUBKEYS);

	printf("[registry scan] There are %d values in key %s\n", num_values, current->name);
	
	if (!num_values) { current->data = NULL; }
	else
	{
		//set values as keys
		//get offset of first value key. this will be stored in a list
		list_address = 0;
		memcpy(&list_address, hive->data + current->offset + VALUE_CELL, ADDRESS);
		list_address = list_address + HEADER + ADDRESS;
		memcpy(address, hive->data + list_address, ADDRESS);
		
		//skip the size
		struct REG_KEY * valuekey = malloc(sizeof(struct REG_KEY));
		struct REG_KEY * next;
		struct REG_KEY * tempkey;
		valuekey->parent = current;
		valuekey->valuekey = NULL; 
		memcpy(&valuekey->offset, address, ADDRESS);
		valuekey->offset += HEADER;
		next = valuekey;
	
		current->valuekey = next;
		
		//loop through all values
		for (int index = 0; index < num_values; index++)
		{
			//set the name
			name_length = 0;
			memcpy(&name_length, hive->data + next->offset + VK_NAME_LENGTH, 2);
			if (name_length == 0)
			{
				name_length = 9;
				next->name = calloc(name_length, sizeof(char));
				sprintf(next->name, "(Default)");
				
			}
			else 
			{
				next->name = calloc(name_length, sizeof(char));
				memcpy(next->name, hive->data + next->offset + VK_KEY_NAME, name_length);
			}
			
			printf("[registry scan] added valuekey %s at address ", next->name);
			for (int i = 0; i < ADDRESS; i++) { printf("%02x", address[i]); }
			printf(" to %s\n", current->name);
			
			//check if the data is resident
			//get the highest order bit to check for residency; 1 = reisdent, 0 = non-resident
			int data_size;
			memcpy(&data_size, hive->data + next->offset + DATA_LENGTH, ADDRESS);
			next->data_size = data_size;
			if (data_size >= RESIDENT)
			{
				//if data is resident, all of it (max 4 bytes) lives at 0xC0
				printf("[registry scan] data is resident\n");
				data_size -= RESIDENT;
				next->data = calloc(data_size, sizeof(unsigned char));
				memcpy(next->data, hive->data + next->offset + DATA_CELL, data_size);
			}
			else
			{
				//if data is non-resident we have to bother with lists and data cells
				int data_address;
				memcpy(&data_address, hive->data + next->offset + DATA_CELL, ADDRESS);
				data_address += HEADER;
				printf("[registry scan] data is at offset %d with size of %d\n", data_address, data_size);
				
				//copy data
				next->data = calloc(data_size, sizeof(unsigned char));
				memcpy(next->data, hive->data + data_address + ADDRESS, data_size);
			}

			//increment address   
			list_address = list_address + ADDRESS;		
			int_address = 0;
			memcpy(address, hive->data + list_address, ADDRESS);
			
			memcpy(&int_address, address, ADDRESS);
			int_address += HEADER;
			
			tempkey = set_next(next, current, int_address);
			next = tempkey;
		}
	}
	
	//navigate to subkey-list cell index, at offset 0x20
	//get the first address (reverse endianness)
	list_address = 0;
	memcpy(&list_address, hive->data + current->offset + SUBKEY_CELL, ADDRESS);
	list_address = list_address + HEADER + ADDRESS + ADDRESS;
	memcpy(address, hive->data + list_address, ADDRESS);
		
	//read the number of subkeys, at relative offset 0x18 in little endian
	int num_subkeys = 0;
	memcpy(&num_subkeys, hive->data + current->offset + SUBKEY_COUNT, NUM_SUBKEYS);
	
	printf("[registry scan] There are %d subkeys in key %s\n", num_subkeys, current->name);
	
	if (!num_subkeys) { current->subkey = NULL; }
	else 
	{	
		//set first subkey
		struct REG_KEY * subkey = malloc(sizeof(struct REG_KEY));
		struct REG_KEY * next;
		struct REG_KEY * tempkey;
		memcpy(&subkey->offset, address, ADDRESS);
		subkey->offset += HEADER;
		
		subkey->name = calloc(MAX_LEN, sizeof(char));
		name_length = 0;
		memcpy(&name_length, hive->data + subkey->offset + NK_NAME_LENGTH, 2);
		memcpy(subkey->name, hive->data + subkey->offset + NK_KEY_NAME, name_length);
		
		printf("[registry scan] added subkey %s at address ", subkey->name);
		for (int i = 0; i < ADDRESS; i++) { printf("%02x", address[i]); }
		printf(" to %s\n", current->name);
		subkey->parent = current;
		next = subkey;
		current->subkey = subkey;
		
		for (int index = 0; index < num_subkeys - 1; index++)
		{
			//increment address 
			list_address = list_address + ADDRESS + ADDRESS;		
			int_address = 0;
			memcpy(address, hive->data + list_address, ADDRESS);
			
			memcpy(&int_address, address, ADDRESS);
			int_address += HEADER;
			tempkey = set_next(next, current, int_address);
			next = tempkey;
			
			//set its name
			//get the name of the cell at relative offset 0x50
			//the length of the name is 2 bytes at offset 0x4C
			next->name = calloc(MAX_LEN, sizeof(char));
			name_length = 0;
			memcpy(&name_length, hive->data + next->offset + NK_NAME_LENGTH, 2);
			
			memcpy(next->name, hive->data + next->offset + NK_KEY_NAME, name_length);
			printf("[registry scan] added subkey %s at address ", next->name);
			for (int i = 0; i < ADDRESS; i++) { printf("%02x", address[i]); }
			printf(" to %s\n", current->name);
		}
	}
	
	current->scanned = 1;
	
	return 1;
}

struct REG_KEY * set_next(struct REG_KEY * prev, struct REG_KEY * parent, uint32_t address)
{
	struct REG_KEY * current = malloc(sizeof(struct REG_KEY));
	
	current->offset = address;
	current->parent = parent;
	current->scanned = 0;
	current->nextkey = NULL;
	current->subkey = NULL;
	current->valuekey = NULL;
	
	prev->nextkey = current;
	
	return current;
}