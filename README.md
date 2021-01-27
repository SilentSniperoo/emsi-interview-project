# emsi-interview-project

The pattern matching project for my Emsi interview.

To compile and run the code in one step, open a terminal and paste:

``` bash
g++ -Wall -Wextra -Werror -Wpedantic -Wconversion -O2 -std=c++17 linefuzzyfinder.cpp -o linefuzzyfinder.out && ./linefuzzyfinder.out
```

You can also run all tests written in a text file one after the other. Each line is a list of space separated words, which are used as the input words for the test that line represents. To do so, use the following:

``` bash
g++ -Wall -Wextra -Werror -Wpedantic -Wconversion -O2 -std=c++17 linefuzzyfinder.cpp -o linefuzzyfinder.out && ./linefuzzyfinder.out -d ./lepanto.txt -i ./testInputs.txt
```

Alternatively, to try out tests with your own words not using a document, use:

``` bash
g++ -Wall -Wextra -Werror -Wpedantic -Wconversion -O2 -std=c++17 linefuzzyfinder.cpp -o linefuzzyfinder.out && ./linefuzzyfinder.out -d ./lepanto.txt -c "his head a flag" "test word set two" "set three"
```
