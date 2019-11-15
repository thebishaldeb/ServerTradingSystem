/* TCPsERver.c */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define MAX_CLIENT 5
#define MAX_MSG 100

#define TRADER_KEY 0x1111
#define ITEM_KEY 0x1112
#define BUY_KEY 0x1113
#define SELL_KEY 0x1114

#define MEM_SZ 4096

struct trader
{
    int id;
    char *name;
    int flag;
};

struct item
{
    int id;
    char *name;
    int bestBuy;
    int bestSell;
    int quantity;
};

struct buyQueue
{
    int userId;
    int buyId;
    int itemId;
    int quantity;
    int price;
    int flag;
};

struct sellQueue
{
    int userId;
    int sellId;
    int itemId;
    int quantity;
    int price;
    int flag;
};

struct tradeQueue
{
    int customerId;
    int sellerId;
    int sellId;
    int itemId;
    int quantity;
    int price;
};

int main(int argc, char *argv[])
{
    int i;

    void *memI = (void *)0;
    struct trader *traders;
    srand((unsigned int)getpid());
    int shmTraderId = shmget((key_t)1234, MEM_SZ, 0666 | IPC_CREAT);
    memI = shmat(shmTraderId, (void *)0, 0);
    traders = (struct trader *)memI;
    for (i = 0; i < 5; i++)
    {
        (traders + i)->id = i + 1;
        (traders + i)->flag = 0;
        int shmTnameId = shmget((key_t)1000 + i, sizeof(char), 0666 | IPC_CREAT);
        (traders + i)->name = (char *)shmat(shmTnameId, 0, 0);
        sprintf((traders + i)->name, "Trader %d", i + 1);
    }

    // Items List
    void *memT = (void *)0;
    struct item *items;
    srand((unsigned int)getpid());
    int shmItemId = shmget((key_t)1256, MEM_SZ, 0666 | IPC_CREAT);
    memT = shmat(shmItemId, (void *)0, 0);
    printf("Memory attached at %X\n", (int)memT);
    items = (struct item *)memT;
    for (i = 0; i < 3; i++)
    {
        items[i].id = i + 1;
        int shmInameId = shmget((key_t)1010 + i, sizeof(char), 0666 | IPC_CREAT);
        (items + i)->name = (char *)shmat(shmInameId, 0, 0);
        sprintf((items + i)->name, "Item %d", i + 1);
        items[i].quantity = 0;
        items[i].bestSell = 0;
        items[i].bestBuy = 0;
    }

    // char *buyKeyString = "buy";
    // char *sellKeyString = "sell";

    int sd, newSd, cliLen, len;
    struct sockaddr_in cliAddr, servAddr;
    char buf[MAX_MSG];
    int optval = 1;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server IP>  <Server Port>\n", argv[0]);
        exit(1);
    }

    /* build socket address data structure */
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(atoi(argv[2]));

    /* create socket for passive open */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        perror("cannot open socket ");
        exit(1);
    }
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));

    if (bind(sd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        perror("cannot bind port ");
        exit(1);
    }

    listen(sd, MAX_CLIENT);

    i = 1;
    pid_t child;
    while (i == 1)
    {
        cliLen = sizeof(cliAddr);
        newSd = accept(sd, (struct sockaddr *)&cliAddr, &cliLen);

        if (newSd < 0)
        {
            perror("cannot accept connection ");
            exit(1);
        }

        child = fork();

        if (child < 0)
        {
            perror("Fork Error");
            exit(1);
        }
        if (child == 0)
        {
            close(sd);
            memset(buf, 0x0, MAX_MSG);
            if (len = read(newSd, buf, sizeof(buf)))
            {
                printf("\nUser request received from client: %s.\n", buf);
                int userId = atoi(buf);
                if (userId < 1 || userId > 5)
                {
                    strcpy(buf, "unknown");
                    printf("\nUser doesn't exist\n");
                    send(newSd, buf, sizeof(buf), 0);
                    printf("\nClosing current connection with Client!\n");
                    close(newSd);
                    close(sd);
                    shmdt(traders);
                    exit(0);
                }
                else if (traders[userId - 1].flag == 1)
                {
                    strcpy(buf, "failed");
                    printf("\nUser logged in from another client!\n");
                    send(newSd, buf, sizeof(buf), 0);
                    printf("\nClosing current connection with Client\n");
                    close(newSd);
                    close(sd);
                    shmdt(traders);
                    exit(0);
                }
                else
                {
                    printf("\nTrader %d logged in!\n", userId);
                    send(newSd, buf, sizeof(buf), 0);
                    traders[userId - 1].flag = 1;
                    if (len = read(newSd, buf, sizeof(buf)))
                    {
                        if (!strcmp(buf, "exit"))
                        {
                            printf("\nTrader %d logged out!\n", userId);
                            traders[i - 1].flag = 0;
                            close(newSd);
                            close(sd);
                            shmdt(traders);
                            exit(0);
                        }
                    }
                    close(newSd);
                    close(sd);
                    shmdt(traders);
                    exit(0);
                }
            }
        }
        else
        {
            close(newSd);
        }
    }
}
