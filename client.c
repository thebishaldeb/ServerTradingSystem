#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

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
    char *traderKeyString, *buyKeyString, *sellKeyString;
    int sd, rc, len;

    struct sockaddr_in localAddr, servAddr;
    char buf[MAX_MSG];
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server IP>  <Server Port> \n", argv[0]);
        exit(1);
    }

    /* build socket address data structure */
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(atoi(argv[2]));

    /* create socket, active open */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        perror("cannot open socket ");
        exit(1);
    }

    /* connect to server */
    rc = connect(sd, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if (rc < 0)
    {
        perror("cannot connect ");
        exit(1);
    }

    printf("\nConnected to the server %s at TCP port %s \n", argv[1], argv[2]);

    printf("\nEnter user id to log in: ");
    scanf("%s", buf);
    send(sd, buf, strlen(buf) + 1, 0);

    if (len = read(sd, buf, sizeof(buf)))
    {
        if (!strcmp(buf, "failed"))
        {
            printf("\nUser logged in from another client!\n");
            close(sd);
            exit(0);
        }
        else if (!strcmp(buf, "unknown"))
        {
            printf("\nUser doesn't exist!\n");
            close(sd);
            exit(0);
        }
        else
        {
            printf("\nLogged you in!\n");
            void *memT = (void *)0;
            struct item *items;
            srand((unsigned int)getpid());
            // char *itemKeyString = "items";
            // key_t itemKey = ftok(itemKeyString, 65);
            int shmItemId = shmget((key_t)1256, MEM_SZ, 0666 | IPC_CREAT);
            memT = shmat(shmItemId, (void *)0, 0);
            printf("Memory attached at %X\n", (int)memT);
            items = (struct item *)memT;
            for (i = 0; i < 3; i++)
            {
                int shmInameId = shmget((key_t)1010 + i, sizeof(char), 0666 | IPC_CREAT);
                (items + i)->name = (char *)shmat(shmInameId, 0, 0);
            }

            while (1)
            {
                printf("\nChoose a number among the following options: ");
                printf("\n\t1 - The status of all the items");
                printf("\n\t2 - Sell an item");
                printf("\n\t3 - Buy an item");
                printf("\n\t4 - Status of your trades");
                printf("\n\t5 - Exit");
                printf("\n\nPlease select your choice: ");
                int choice;
                scanf("%d", &choice);
                switch (choice)
                {
                case 1:
                    for (i = 0; i < 3; i++)
                    {
                        printf("\nItem %d (%s) - %d no of items", (items + i)->id, (items + i)->name, (items + i)->quantity);
                        printf("\n\tBest buy: %d", (items + i)->bestBuy);
                        printf("\n\tBest Sell: %d\n", (items + i)->bestSell);
                    }
                    break;

                case 5:
                    printf("\nLogged you out!\nExiting...\n");
                    sprintf(buf, "exit");
                    send(sd, buf, strlen(buf) + 1, 0);
                    exit(0);
                    break;
                default:
                    printf("\nPlease try again!");
                    break;
                }
            }
        }
    }
    printf("\nTask Accompolished!!\n");
}
