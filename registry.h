#ifndef REGISTRY_H
#define REGISTRY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef _WIN32
  #define strcasecmp _stricmp
#else
  #include <string.h>
#endif

#define NUM_SUBKEYS 	4
#define ADDRESS 		4
//define known offsets
#define HEADER			0x1000
#define SUBKEY_COUNT	0x18
#define SUBKEY_CELL		0x20
#define VALUE_COUNT		0x28
#define VALUE_CELL		0x2C
#define NK_NAME_LENGTH	0x4C
#define NK_KEY_NAME		0x50
#define VK_KEY_NAME		0x18
#define DATA_CELL		0x0C
#define VALUE_SIZE		0x00
#define VK_NAME_LENGTH	0x06
#define DATA_LENGTH		0x08

#define MAX_LEN 		1024
#define RESIDENT 		0x80000000

//structure to hold the registry hive
//contains the offset of the root key and the entire hive as raw data
//needs special traversat functions to traverse
struct REGISTRY_HIVE
{
	char * name;
	struct REG_KEY * root;
	uint8_t * data;
};

//structure to keep track of a registry key and associated values
struct REG_KEY
{
	char * name;
	int scanned;
	uint32_t offset;
	unsigned char * data;
	uint32_t data_size;
	struct REG_KEY * nextkey;
	struct REG_KEY * parent;
	struct REG_KEY * subkey;
	struct REG_KEY * valuekey;
};

//open a registry hive
//returns 1 on success, 0 otherwise. Outputs hive to provided address
int load_hive(char * hive_path, struct REGISTRY_HIVE ** hive);

//close a registry hive
//delete all the REG_KEYs that I've created in there or valgrind will have me killed
//returns 1 on success, 0 otherwise
int close_hive(struct REGISTRY_HIVE * hive);

//recursive helper function to free the tree of keys I've built
void registry_free(struct REG_KEY * current);

//query the value of a key
//returns 1 on success and 0 on failure
int registry_query_key(struct REGISTRY_HIVE * hive, char * target_key, unsigned char ** data);

//make reg_key data type that stores the current path and all values for that key
//walk down the registry recursively until we find the target key
int registry_walk(struct REGISTRY_HIVE * hive, struct REG_KEY * current_key, char * target_key, int level, struct REG_KEY ** out);

//maps all values in a key
//returns pointer to the first value, and sets each subsequent as a pointer
//sets the provided root as the parent, keeps subkey as NULL
//determine the structure of a registry key as in headers and bullshit
int registry_scan_keys(struct REGISTRY_HIVE * hive, struct REG_KEY ** root);

struct REG_KEY * set_next(struct REG_KEY * prev_key, struct REG_KEY * parent, uint32_t address);

#endif