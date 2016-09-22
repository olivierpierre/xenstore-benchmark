# xenstore-benchmark

## Description and usage
There is a simple benchmark to evaluate the performance of the xenstore
located in tools/xenstore/xenstore-benchmark. Compile it simply with
"make".

To launch the benchmark:
`./xenstore-benchmark <config file>`

An exemple of config file is the file `config.cfg`.

The benchmark targets the `read`, `write`, `close`, `open`, 
`get_domain_path` and `set_permission` xenstore operations. For each 
of these operations, it calls the operation several times (N times) in
a loop and reccord the average and stdev values. You can configure 
N for each operation in the config file for example below the loop 
for xs_read iterates 5000 times. Each loop is repeated M times, this
is configured with the repeat key in the config file. Finally, you can
customize the size of each read / write request in bytes with the 
req_size key.

## Configuration file
Exemple:

```
connection_num 20

xs_read 5000
xs_write 5000

xs_close 100
xs_open 100

xs_get_domain_path 1000
xs_set_permission 1000

repeat 10
req_size 32

print_config 1
```

* `connection_num` represents the number of connections open in the background while performing the benchmark
* `xs_read`, `xs_write`, etc. represent the number of operation of each type performed in a loop during the benchmark
* `repeat` is the number of times each loop is repeated (average and stdev for the execution time of an operation type loop are computed based on these values)
* `req_size` is the size of read / written data for read / write operations.
* `print_config` is set to 1 to print the configuration at runtime, 0 otherwise.
 
## Output
Exemple:
``` shell
./xenstore-benchmark config.cfg 
Configuration:
--------------
connection_num: 0
xs_read: 500
xs_write: 500
xs_open: 0
xs_close: 0
xs_get_domain_path: 500
xs_set_permission: 0
repeat: 10
req_size: 32
--------------
write;63735.800000;4911.874363
read;50625.600000;1982.256855
get_domain_path;37099.400000;1727.733961
```

First the configuration is printed if the `print_config` parameter is enabled in the configuration file. Below, each line corresponds to an operation with the following fields in left to right order:
* The operation name
* The average execution time of the loop (repeated `repeat` times)
* The standard deviatin
