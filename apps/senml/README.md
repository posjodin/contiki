# senml app
The senml app is an API for creating SenML messages in JSON or CBOR format.  
It handles the formatting of the SenML pack conforming to [RFC 8428](https://tools.ietf.org/html/rfc8428). It can also be used to create SenSML messages.

## Project code needs

```c
#include "senml-json.h"
```  
and/or  
```c
#include "senml-cbor.h"
```  
depending on desired format

## Build

To include in project:
in Makefile add

```
APPS += senml
```

You also need to link in a library for printing floating point numbers, such as `libprintf_flt` for AVR if you are using the JSON format.

## Use

API is used through different macros:  
`SENML_INIT_JSON` or `SENML_INIT_CBOR`,  
`SENML_START_PACK`,  
`SENML_ADD_RECORD`,  
and `SENML_END_PACK`.  

The SenML message is written into the buffer given as an argument.
`SENML_ADD_RECORD` also takes field name-value pairs as arguments. The supported fields are:

| Field name    | Data type |
| ------------- |:---------:|
| BASE_NAME     | char *    |
| BASE_TIME     | float     |
| BASE_UNIT     | char *    |
| BASE_VALUE    | float     |
| BASE_SUM      | float     |
| BASE_VERSION  | int       |
| NAME          | char *    |
| UNIT          | char *    |
| VALUE         | float     |
| STRING_VALUE  | char *    |
| BOOLEAN_VALUE | int(*)    |
| DATA_VALUE    | char *    |
| SUM           | float     |
| TIME          | float     |
| UPDATE_TIME   | float     |

(*) 0 = false, all other values = true

Maximum int value and string length is 65535 in CBOR formatter (can be extended if needed).

Note: When using JSON format, floating point numbers are printed using `%g` format with a precision of 6 digits. This can result in rounding errors.

### Macros
```c
// Initializes SenML API to use JSON format
void SENML_INIT_JSON()
```
```c
// Initializes SenML API to use CBOR format
void SENML_INIT_CBOR()
```
```c
// Begins a new SenML message in the given buffer and returns the number of characters written.
int SENML_START_PACK(buf_ptr, buf_len)
```
```c
// Adds a record with the given fields in the given buffer and returns the number of characters written.
// For example 
// ADD_RECORD(buf_ptr, buf_len, BASE_NAME, "name", BASE_UNIT, "unit", VALUE, 4.6)
// adds a record with the fields bn = name, bu = unit, v = 4.6
int SENML_ADD_RECORD(buf_ptr, buf_len, ...)
```
```c
// Ends the SenML message in the given buffer and returns the number of characters written.
// Shall not be called in the beginning of a buffer if using JSON since it needs to step backwards in the buffer and overwrite the last record separation character.
int SENML_END_PACK(buf_ptr, buf_len)
```

## Example usage
```c
#define BUFFER_SIZE 1024

static char buffer[BUFFER_SIZE];
char* buf_ptr = buffer;
int len = 0;

SENML_INIT_JSON();
len += SENML_START_PACK(buf_ptr + len, BUFFER_SIZE - len);
len += SENML_ADD_RECORD(buf_ptr + len, BUFFER_SIZE - len, BASE_NAME, "urn:dev:ow:10e2073a01080063", NAME, "voltage", UNIT, "V", VALUE, 120.1);
len += SENML_ADD_RECORD(buf_ptr + len, BUFFER_SIZE - len, NAME, "current", UNIT, "A", VALUE, 1.2);
len += SENML_END_PACK(buf_ptr + len, BUFFER_SIZE - len);

printf("%s", buffer);
```
Should print
```json
[{"bn":"urn:dev:ow:10e2073a01080063:","n":"voltage","u":"V","v":120.1},{"n":"current","u":"A","v":1.2}]
```
## Code structure
The different lables are defined in `label.h`.  
The main code that handles the different labels is in `senml-api.c`.  
Formatting is handled in `senml-cbor-fomatter.c` and `senml-json-formatter.c`, both implementing the functions defined in `senml-formatter.h`  
User macros defined in `senml-json.h` and `senml-cbor.h` provide a formatter to the main functions in `senml-api.c`.  

## Future work
Implementation of XML and EXI.  
Add support for double- and half-precision floating point numbers.  

## Authors
Erik Flink   \
Isak Olsson \
Nelly Friman \
Anton Bothin   \
Andreas Sjödin \
Jacob Klasmark  \
Carina Wickström \
Valter Lundegårdh 
