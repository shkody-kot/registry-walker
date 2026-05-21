## Registry Walker

Rudimentary application to fetch values from Windows Registry keys. Cannot function on a live registry (the file is protected by the system) and needs an offline hive. 
Pass the path to the walker and query various Registry keys. <br>


This walker can fetch hidden class information via `registry_query_key_class` but that is not currently an option in the main function; this information is used by tools
such as Mimikatz or Impacket to fetch the pieces of the Syskey (may also be called the boot key) from four different keys in the SYSTEM hive. The boot key is needed to 
grab and decrypt LSA secrets and NT hashes of user passwords. 

#### Usage

To make the executable, run `make`. No external libraries are required to run it outside of C and gcc, and make. <br>

To run: `./registry-walker`. The program will prompt for the path to the hive. 

