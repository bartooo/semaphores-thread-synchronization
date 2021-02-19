# Thread synchronization with use of semaphores

## Table of Contents

- [About](#about)
- [Usage](#usage)

## About <a name = "about"></a>

Project solves the Producer-Consumer problem using semaphores. There are three types of Users:
- VIP Users (Producer)
- Normal Users (Producer)
- Reader (Consumer)

Messages sent by VIP Users have higher priority than messages sent by Normal Users.

This solution uses three semaphores:
- one called `sem_full` for counting the number of slots that are full in buffer
- one called `sem_empty` for counting the number of slots that are empty in buffer
- one called `sem_mutex` to make sure the Producer and Consumer do not access the buffer at the same time

### Initializing of queue

Queue is initialized in function `init_message_queue()`. 

User can set maximal number of messages in queue.
```C
#define MAX_MESSAGES_BUFF
```
User can also set maximal number of signs in every message.
```C
#define MAX_MESSAGE_SIZE
```

## Usage <a name = "usage"></a>

To run a test, user needs to uncomment particular test function, which are called from `main()` function in file `semaphores.c`.

Compiling `semaphores.c`:
```
cc -o semaphores semaphores.c -lpthread
```
```
./semaphores
```