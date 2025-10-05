# ASCII 85 (de)coder!
## Description
Simple ASCII85-encoder/decoder. Seems to be working!

## Requirements
* `C++ 20`
* `make`
* `gtest`

## Build 
``` Bash
make 
```
.. and one (or not) from available targets:
* `all`     -   Build the main ASCII85 encoder/decoder program
* `test`    -   Build and run unit tests
* `clean`   -   Remove all built files
* `help`    -   Show help message

## Run Examples
`echo 'Hello World' | ./ascii85`                -   Encode (default)
`echo 'Hello World' | ./ascii85 -e`             -   Encode explicitly
`echo '87cURD]j7BEbo80~>' | ./ascii85 -d`       -   Decode
`./ascii85 --stream < input.txt > output.txt`   -   Stream mode

## Author and Contacts
Andikalovskiy Nikita, 24.B82-mm, 
st131335@student.spbu.ru