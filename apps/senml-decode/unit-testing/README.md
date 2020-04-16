# Unit testing for SenML-decode API
Unit tests for the SenML-decode API using the unit-test app.
## How to run
Create the build using make. Native will be used by default if no target is specified.
```
make
./unit-testing.native
```
## Use
### Registering new tests
In unit-testing.c, register new tests by adding
```UNIT_TEST_REGISTER(name, "description")```
in the start of the file.

Run the test by adding ```UNIT_TEST_RUN(name);``` in the ```PROCESS_THREAD``` method:
### Example test design
```
UNIT_TEST(name) {
  int a, b;
  UNIT_TEST_BEGIN();
  a = 1;
  b = 2;
  UNIT_TEST_ASSERT(a+b==3);
  UNIT_TEST_END();
}
```
This test will pass since 1+2=3
## Common errors
**make can't find curses.h**  
`sudo apt-get install libncurses5-dev libncursesw5-dev`
## Authors
Anton Bothin  
Erik Flink  
Nelly Friman  
Jacob Klasmark  
Valter Lundegårdh  
Isak Olsson  
Andreas Sjödin  
Carina Wickström  
