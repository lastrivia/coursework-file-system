# Test Guide of Project 3

## Build

```shell
make
```
This will generate all executable files for the test.

---

## Step 1

### Initialization

First, launch the virtual disk server. The command format is:

```shell
disk filename -c cylinders -s sectors_per_cylinder [-b bytes_per_sector=256] [-d delay_us=0] -p port
```

For example, run

```shell
./disk disk.raw -c 16 -s 16 -p 10001
```

will create a virtual disk file `disk.raw` of 64KB, and start the virtual disk server  on the port `10001`.

Next, run the shell client for raw disk operations. The command format is:

```shell
client port
```
For example, run

```shell
./client 10001
```

will connect the client to the server we have created before.

### Test

We can input the commands for raw disk operations to the client, and see the output. For example:

```
raw$ I
16 16
raw$ R 0 0
Yes 
raw$ W 0 0 abcd
Yes
raw$ R 0 0
Yes abcd
raw$ R 16 0
No
raw$ E
Goodbye!
```

### Exceptions

Here lists some common cases of exception:

* Using an invalid or incomplete command. The program will print a prompt and exit.

```
./disk
```

```
Usage: disk filename -c cylinders -s sectors_per_cylinder [-b bytes_per_sector=256] [-d delay_us=0] -p port 
```

* The disk file cannot be created (e.g. the specified size is too large).

```
./disk 1.raw -c 1048576 -s 1048576 -p 10000
```
```
[ERROR] 770: Failed to create virtual drive file (Invalid argument)
```

* The specified port of the server is in use.

```
./disk 1.raw -c 16 -s 16 -p 10000
./disk 2.raw -c 16 -s 16 -p 10000
```
```
[ERROR] 257: Socket creation failed (Address already in use)
```

* The input commands to the client is invalid.

```
raw$ W a
No
raw$ aaa
Unknown command: aaa
raw$ 
```


---

## Step 2

### Initialization

First, initialize the file system server. The command format is:

```shell
fs port
```

For example, run

```shell
./fs 10002
```

will start the file system server on the port `10002`.

Then, run the shell client for file system operations. The command format is:

```shell
client port
```
For example, run

```shell
./client 10002
```

will connect the client to the file system server we created.

### Test

Try different commands for raw disk operations to the client, and see the output. For example:

```
Disk formatted on the first run
file-system:/$ mkdir abc
file-system:/$ mk def
file-system:/$ ls
abc	def
file-system:/$ cd abc
file-system:/abc/$ mk tst
file-system:/abc/$ cat tst

file-system:/abc/$ w tst 123456789
file-system:/abc/$ cat tst
123456789
file-system:/abc/$ i tst 5 aaaaa
file-system:/abc/$ cat tst
12345aaaaa6789
file-system:/abc/$ d tst 2 5
file-system:/abc/$ cat tst
12aaa6789
file-system:/abc/$ 
```

Also, we can connect another client to the file system:

```
file-system:/$ ls
abc	def
file-system:/$ rm def
file-system:/$ mk xyz
file-system:/$ rmdir abc
Failed: the directory is in use
file-system:/$ mkdir abc
Failed: directory abc already exists
file-system:/$ rm def
Failed: file def not found
file-system:/$ 
```

The changes performed by the second client can be checked via the first client:

```
file-system:/abc/$ cd ..
file-system:/$ ls
abc	xyz
file-system:/$ 
```

### Exceptions

Here lists some common cases of exception:

* Using an invalid or incomplete command. The program will print a prompt and exit.

```
./fs
```

```
Usage: fs port
```

* The client sends an instruction to a file system process that has been terminated. The client will exit then.

```
[ERROR] 261: Socket disconnected
```

* The input commands to the client is invalid.

```
file-system:/$ rm
Missing file name
file-system:/$ rrrmmm 
Unknown command: rrrmmm
```

---

## Step 3

First, start the disk server like Step 1. Then, run the file system server and clients like Step 2. Note that we need to connect the file system to the virtual disk, so the command is:

```
fs disk_port port
```

For example, run

```
./fs 10001 10002
```
will start the file system server on the port `10002`, while it connects to the virtual disk server on `10001`.

The file system can be tested in the same way as Step 2.

We can also relaunch the disk and the file system to test if the data is persistent:

```
Disk formatted on the first run
file-system:/$ mkdir a
file-system:/$ mk b
file-system:/$ ls
a	b
file-system:/$ w a 123
```

On the next run:

```
file-system:/$ ls
a	b
file-system:/$ cat a
123
```

---

## Clean

When all tests are finished, using

```shell
make clean
```

can clean temporary files generated during the test.
