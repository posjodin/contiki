# senml-decode app
The senml-decode app is an API for decoding SenML messages in JSON format.  
It uses the JSON parser already implemented in Contiki and supports streaming. 

## Project code needs

```c
#include "senml-decode.h"
```  

## Build

To include in project:
in Makefile add

```
APPS += senml-decode
APPS += json 
```

## Use

API is used through three different functions:  
`init_json_decode(char* msg)`, `read_next_token(struct pair* token)` and `add_new_msg(char* msg)`. 

The SenML message that is to be decoded is written into the JSON parser with `init_json_decode`. It then reads the next label-value pair with `read_next_token` and places the label and value in the struct given as the argument. With `add_new_msg` it is also possible to add further messages to the already existing one. 

### Functions 
```c
// Creates a jsonparse struct that is used to parse the message 
// passed as an argument to the function. 

// It sets the inital values and reads the first two chars 
// of the message which should be "[" and "{". 
void init_json_decode(char* msg)
```
```c
// Reads the next label-value pair in the message passed as an argument 
// in init_json_decode and sets the struct pair* token's members to 
// the label and value read. 

// The struct pair* token is created by the user. 

// If it reads a "}" followed by a "]" it will assume the message 
// is complete and will set the pair struct's members to NULL. 
void read_next_token(struct pair* token)
```
```c
//Takes a message and adds it to the json message currently being parsed. 
void add_new_msg(char* msg)

```

## Example usage

```c
init_json_decoder("[{\"bn\": \"urn:mac:fcc23d000001856e\"}, {"v": 0}");
struct pair lv;

read_next_token(&lv);
printf("result.label: %s\n", lv.label);
printf("result.value: %s\n", lv.value);

add_new_msg(",{\"u\": \"dB\"}]");

read_next_token(&lv);
printf("\n");
printf("result.label: %s\n", lv.label);
printf("result.value: %s\n", lv.value);

read_next_token(&lv);
printf("\n");
printf("result.label: %s\n", lv.label);
printf("result.value: %s\n", lv.value);

read_next_token(&lv)
printf("\n");
printf("result.label: %s\n", lv.label);
printf("result.value: %s\n", lv.value);
```
Should print
```
result.label: bn
result.value: urn:mac:fcc23d000001856e

result.label: v 
result.value: 0 

result.label: u 
result.value: dB

result.label: 
result.value:

```
## Code structure
Since the API is based on the JSON parser already implemented in C the code is split up in 2 parts. 
The main part is in `senml-decode.c` but it heavily relies on the code in `jsonparser.c` in the json app. 

## TODO
Right now the `read_next_token` only reads values as strings. In the future an implementation that can read different values such as integers and doubles is needed. 

The current API also does not inform the user if the end of the message has been read, that is a "}" followed by a "]". Instead it requires the user to continously check if label and value has been set to NULL to see if there are no more messages. A functionality that informs the user of this could be useful so that the user does not try to read after the end has been reached.  

## Authors
Anton Bothin  
Erik Flink  
Nelly Friman  
Jacob Klasmark  
Valter Lundegårdh  
Isak Olsson  
Andreas Sjödin  
Carina Wickström
