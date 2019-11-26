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

#define MEM_SZ 4096

#define TRADER_KEY 0x1000
#define ITEM_KEY 0x1100
#define BUY_KEY 0x1200
#define SELL_KEY 0x1300
#define TRADE_KEY 0x1400

struct trader
{
    int id;
    char *name;
    int flag;
};

struct item
{
    int id;
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
};

struct sellQueue
{
    int userId;
    int sellId;
    int itemId;
    int quantity;
    int price;
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

    /* build socket address data structure */
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(7076);

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

    printf("\nConnected to the server 127.0.0.1 at TCP port 7076 \n");

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
            int shmItemId = shmget((key_t)ITEM_KEY, sizeof(struct item), 0666 | IPC_CREAT);
            memT = shmat(shmItemId, (void *)0, 0);
            items = (struct item *)memT;

            while (1)
            {
                printf("\nChoose a number among the following options: ");
                printf("\n\t1 - The status of all the items");
                printf("\n\t2 - Sell an item");
                printf("\n\t3 - Buy an item");
                printf("\n\t4 - Status of your trades");
                printf("\n\t5 - Exit");
                printf("\n\nPlease select your choice: ");
                int choice, quantity, price, itemId;
                scanf("%d", &choice);
                switch (choice)
                {
                case 1:
                    printf("\nDetails  of the trade items:");
                    for (i = 0; i < 3; i++)
                    {
                        printf("\n\tItem %d - %d no of items", items[i].id, items[i].quantity);
                        printf("\n\t\tBest buy: %d", items[i].bestBuy);
                        printf("\n\t\tBest Sell: %d\n", items[i].bestSell);
                    }
                    break;
                case 2:
                    printf("\nEnter the details of the item you want to sell:");
                    printf("\nEnter the item id, price offered, and no of units: ");
                    scanf("%d%d%d", &itemId, &price, &quantity);
                    if (itemId < 1 || itemId > 5)
                    {
                        printf("\nItem doesn't exist. Please try again!");
                        break;
                    }
                    sprintf(buf, "1 %d %d %d ", itemId, price, quantity);
                    send(sd, buf, strlen(buf) + 1, 0);
                    printf("\nRequest sent\n");
                    break;

                case 3:
                    printf("\nEnter the details of the item you want to buy:");
                    printf("\nEnter the item id, price offered, and no of units: ");
                    scanf("%d%d%d", &itemId, &price, &quantity);
                    if (itemId < 1 || itemId > 5)
                    {
                        printf("\nItem doesn't exist. Please try again!");
                        break;
                    }
                    sprintf(buf, "2 %d %d %d ", itemId, price, quantity);
                    send(sd, buf, strlen(buf) + 1, 0);
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
