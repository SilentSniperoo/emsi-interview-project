# emsi-interview-project

The pattern matching project for my Emsi interview.

To compile and run the code in one step, open a terminal and paste:

``` bash
g++ -Wall -Wextra -Werror -Wpedantic -Wconversion -O2 -std=c++17 linefuzzyfinder.cpp -o linefuzzyfinder.out && ./linefuzzyfinder.out -d ./lepanto.txt -i ./testInputs.txt
```

The above will run all tests written in the "testInputs.txt" file. Each line is a list of space separated words, which are used as the input words for the test that line represents.

Alternatively, to try out tests with your own words, use:

``` bash
g++ -Wall -Wextra -Werror -Wpedantic -Wconversion -O2 -std=c++17 linefuzzyfinder.cpp -o linefuzzyfinder.out && ./linefuzzyfinder.out -d ./lepanto.txt -c "his head a flag" "test word set two" "set three"
```
