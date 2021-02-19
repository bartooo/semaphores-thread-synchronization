#include <malloc.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

#define MAX_MESSAGES_BUFF 10
#define MAX_MESSAGE_SIZE 50


pthread_mutex_t sem_mutex;
sem_t sem_empty;
sem_t sem_full;

int READ_MESSAGES = 0;
int SEND_MESSAGES = 0;
int TOTAL_MESSAGES;

struct queue
{
    int current_size;
    int first_normal_message;
    int max_size;
    char** array;
};


struct queue* queue;


void print_message(int id, int max_message_size)
{
    if (id >= queue->current_size)
    {
        printf("Invalid message ID:%d\n",id);
    }
    else
    {
        // Check if user does not want to read whole message
        if (max_message_size < strlen(queue->array[id]))
        {
            char* c;
            int index;
            c = strchr(queue->array[id], ']');
            index = (int)(c - queue->array[id]);
            char subbuff[max_message_size + index + 2];
            memcpy(subbuff, queue->array[id], max_message_size + index + 1);
            subbuff[max_message_size + index + 1] = '\0';
            queue->array[id] = subbuff;
            printf("%s\n", queue->array[id]);
        }
        else
        {
            printf("%s\n", queue->array[id]);
        }
        
    }
}

void print_queue()
{
    printf("|");
    for (int i = 0; i < queue->current_size - 1; i++)
    {
        printf("%s-", queue->array[i]);;
    }
    printf("|\n");
}

int init_message_queue (int queue_size, int queue_entity_size)
{
    int i;
    
    queue = malloc(sizeof(struct queue));   // Allocate memory for queue structure
    if (queue == NULL)
    {
        return -1;
    }

    queue->current_size = 0;
    queue->first_normal_message = 0;
    queue->max_size = queue_size;
    queue->array = malloc(queue_size * sizeof(char*));  // Allocate memory for array of pointers
    for (i = 0; i < queue_size; i++)
    {
        queue->array[i] = NULL;
    }

    return 0;
}


int put_vip_message(char *message, int message_size)
{
    sem_wait(&sem_empty);   // Assure that message will not be putted into full buffor
    pthread_mutex_lock(&sem_mutex); // Assure that only one operation will be hold
    
    // Shift all normal messages forward
    for (int i = queue->current_size; i > queue->first_normal_message ;i--)
    {
        queue->array[i] = queue->array[i - 1];
    }

    queue->array[queue->first_normal_message] = NULL;
    queue->array[queue->first_normal_message] = malloc( message_size * sizeof(char));

    // Concatenate message with number indicating when message was putted to buffor
    char* msg_with_number = malloc(message_size * sizeof(char) + 5);
    sprintf(msg_with_number, "[%d]", SEND_MESSAGES);
    strcat(msg_with_number, message);

    // Put message between all other VIP messages and all normal messages
    queue->array[queue->first_normal_message] = msg_with_number;
    queue->current_size++;
    queue->first_normal_message++;

    SEND_MESSAGES++;
    pthread_mutex_unlock(&sem_mutex);
    sem_post(&sem_full);
    return 0;
}

int put_normal_message(char *message, int message_size)
{  
    sem_wait(&sem_empty);   // Assure that message will not be putted into full buffor 
    pthread_mutex_lock(&sem_mutex); // Assure that only one operation will be hold
    queue->array[queue->current_size] = malloc( message_size * sizeof(char));
    if (queue->array[queue->current_size] == NULL)
    {
        return -1;
    }

    // Concatenate message with number indicating when message was putted to buffor
    char* msg_with_number = malloc(message_size * sizeof(char) + 5);
    sprintf(msg_with_number, "[%d]", SEND_MESSAGES);
    strcat(msg_with_number, message);

    // Put message at the end of the queue
    queue->array[queue->current_size] = msg_with_number;
    queue->current_size++;

    SEND_MESSAGES++;
    pthread_mutex_unlock(&sem_mutex);
	sem_post(&sem_full);    // Signals that there is one more message in buffor
    return 0;
}

int get_message(int max_message_size)
{
    sem_wait(&sem_full);    // Assure that buffor is not empty
    pthread_mutex_lock(&sem_mutex); 

    print_message(0, max_message_size);

    int i = 0;

    // Shift all messages backward
    while(i < queue->current_size - 1)
    {
        queue->array[i] = queue->array[i+1];
        i++;
    }
    queue->array[i] = NULL;
    queue->current_size--;
    if (queue->first_normal_message - 1 >= 0)
    {
        queue->first_normal_message--;
    }

    READ_MESSAGES++;
    pthread_mutex_unlock(&sem_mutex);
    sem_post(&sem_empty);   // Signal that there is one more empty place in buffor
    return 0;
}

void* vip_user(void* param)
{
    printf("VIP User created\n");
    for (int i = 0; i < 20; i++)
    {
        sleep(rand() % 2);
        char* message = "VIP";
        put_vip_message(message, strlen(message));
    }
}

void* normal_user1(void* param)
{
    printf("Normal User 1 created\n");
    for (int i = 0; i < 20; i++)
    {
        sleep(rand() % 2);
        char* message = "Normal user 1";
        put_normal_message(message, strlen(message));
    }

}

void* normal_user2(void* param)
{
    printf("Normal User 2 created\n");
    for (int i = 0; i < 20; i++)
    {
        sleep(rand() % 2);
        char* message = "Normal user 2";
        put_normal_message(message, strlen(message));
    }
}

void* reader(void* param)
{
    printf("Reader created\n");
    while (READ_MESSAGES < TOTAL_MESSAGES)
    {
        sleep(rand() % 2);
        get_message(MAX_MESSAGE_SIZE);
    }
}

void test_both_users(void)
{
    TOTAL_MESSAGES = 60;
    pthread_t user1_th, user2_th, vip_th, reader_th;
    
    pthread_create(&user1_th, NULL, normal_user1, NULL);
    pthread_create(&user2_th, NULL, normal_user2, NULL);
    pthread_create(&vip_th, NULL, vip_user, NULL);
    pthread_create(&reader_th, NULL, reader, NULL);

    pthread_join(user1_th, NULL);
    pthread_join(user2_th, NULL);
    pthread_join(vip_th, NULL);
    pthread_join(reader_th, NULL);
}

void test_normal_users(void)
{
    TOTAL_MESSAGES = 40;
    pthread_t user1_th, user2_th, reader_th;

    pthread_create(&user1_th, NULL, normal_user1, NULL);
    pthread_create(&user2_th, NULL, normal_user2, NULL);
    pthread_create(&reader_th, NULL, reader, NULL);

    pthread_join(user1_th, NULL);
    pthread_join(user2_th, NULL);
    pthread_join(reader_th, NULL);
}

void test_vip_users(void)
{
    TOTAL_MESSAGES = 40;
    pthread_t vip1_th, vip2_th, reader_th;

    pthread_create(&vip1_th, NULL, vip_user, NULL);
    pthread_create(&vip2_th, NULL, vip_user, NULL);
    pthread_create(&reader_th, NULL, reader, NULL);

    pthread_join(vip1_th, NULL);
    pthread_join(vip2_th, NULL);
    pthread_join(reader_th, NULL);
}

void test_empty_buffor_normal(void)
{
    TOTAL_MESSAGES = 20;
    pthread_t reader_th, user1_th;

    pthread_create(&reader_th, NULL, reader, NULL);
    sleep(5);
    pthread_create(&user1_th, NULL, normal_user1, NULL);
    pthread_join(reader_th, NULL);
    pthread_join(user1_th, NULL);
}

void test_empty_buffor_vip(void)
{
    TOTAL_MESSAGES = 20;
    pthread_t reader_th, vip_th;

    pthread_create(&reader_th, NULL, reader, NULL);
    sleep(5);
    pthread_create(&vip_th, NULL, vip_user, NULL);
    pthread_join(reader_th, NULL);
    pthread_join(vip_th, NULL);
}

void test_full_buffor_normal(void)
{
    TOTAL_MESSAGES = 20;
    pthread_t user1_th, reader_th;

    pthread_create(&user1_th, NULL, normal_user1, NULL);
    sleep(10);
    pthread_create(&reader_th, NULL, reader, NULL);
    pthread_join(user1_th, NULL);
    pthread_join(reader_th, NULL);
}

void test_full_buffor_vip(void)
{
    TOTAL_MESSAGES = 20;
    pthread_t vip_th, reader_th;

    pthread_create(&vip_th, NULL, vip_user, NULL);
    sleep(10);
    pthread_create(&reader_th, NULL, reader, NULL);
    pthread_join(vip_th, NULL);
    pthread_join(reader_th, NULL);
}



int main(int argc, char* argv[])
{
    init_message_queue(MAX_MESSAGES_BUFF, MAX_MESSAGE_SIZE);

    pthread_mutex_init(&sem_mutex, NULL);
    sem_init(&sem_empty, 0, queue->max_size);
    sem_init(&sem_full, 0, 0);

    test_both_users();
    //test_normal_users();
    //test_vip_users();
    //test_empty_buffor_normal();
    //test_empty_buffor_vip();
    //test_full_buffor_normal();
    //test_full_buffor_vip();
    
    

    pthread_mutex_destroy(&sem_mutex);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    free(queue);
    return 0;
}