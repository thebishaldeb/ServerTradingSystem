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

#define TRADER_KEY 0x1000
#define ITEM_KEY 0x1100
#define BUY_KEY 0x1200
#define SELL_KEY 0x1300
#define TRADE_KEY 0x1400

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
    int bestBuy;
    int bestSell;
    int quantity;
};

struct buyQueue
{
    int userId;
    int itemId;
    int quantity;
    int price;
    struct buyQueue *next;
};

struct sellQueue
{
    int userId;
    int itemId;
    int quantity;
    int price;
    struct sellQueue *next;
};

struct tradeQueue
{
    int customerId;
    int sellerId;
    int sellId;
    int itemId;
    int quantity;
    int price;
    struct tradeQueue *next;
};

int main(int argc, char *argv[])
{
    int i, j;
    char buffer[100];
    void *memI = (void *)0;
    struct trader *traders;
    srand((unsigned int)getpid());
    int shmTraderId = shmget((key_t)TRADER_KEY, sizeof(struct trader), 0666 | IPC_CREAT);
    memI = shmat(shmTraderId, (void *)0, 0);
    traders = (struct trader *)memI;
    for (i = 0; i < 5; i++)
    {
        (traders + i)->id = i + 1;
        (traders + i)->flag = 0;
        int shmTnameId = shmget((key_t)(TRADER_KEY + i), sizeof(char), 0666 | IPC_CREAT);
        (traders + i)->name = (char *)shmat(shmTnameId, 0, 0);
        sprintf((traders + i)->name, "Trader %d", i + 1);
    }

    // Items List
    void *memT = (void *)0;
    struct item *items;
    srand((unsigned int)getpid());
    int shmItemId = shmget((key_t)ITEM_KEY, sizeof(struct item), 0666 | IPC_CREAT);
    memT = shmat(shmItemId, (void *)0, 0);
    items = (struct item *)memT;
    for (i = 0; i < 3; i++)
    {
        (items + i)->id = i + 1;
        (items + i)->quantity = 0;
        (items + i)->bestSell = 0;
        (items + i)->bestBuy = 0;
    }

    struct buyQueue *buyhead, *tempBuy = NULL, *buyList;
    struct sellQueue *sellhead, *tempSell = NULL, *sellList;

    int quantity, price, itemId;
    int quantityTemp, priceTemp, itemIdTemp, userIdTemp;

    int sd, newSd, cliLen, len;
    struct sockaddr_in cliAddr, servAddr;
    char buf[MAX_MSG];
    int optval = 1;

    /* build socket address data structure */
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port = htons(7076);

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
    while (1)
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
                    shmdt(traders);
                    exit(0);
                }
                else
                {
                    printf("\nTrader %d logged in!\n", userId);
                    send(newSd, buf, sizeof(buf), 0);
                    traders[userId - 1].flag = 1;
                    int textchecker;
                    while (len = read(newSd, buf, sizeof(buf)))
                    {
                        if (!strcmp(buf, "exit"))
                        {
                            printf("\nTrader %d logged out!\n", userId);
                            traders[i - 1].flag = 0;
                            close(newSd);
                            shmdt(traders);
                            exit(0);
                        }
                        else
                        {
                            sscanf(buf, "%d ", &textchecker);
                            if (textchecker == 1) // SELL
                            {
                                sscanf(buf, "%*d %d %d %d ", &itemId, &price, &quantity);
                                printf("\nSale requested of %d no of item %d at %d by trader %d\n", quantity, itemId, price, userId);
                                FILE *buy = fopen("buy.txt", "r"),
                                     *sell = fopen("sell.txt", "a"),
                                     *trade = fopen("trades.txt", "a");
                                buyhead = (struct buyQueue *)malloc(sizeof(struct buyQueue));
                                buyList = buyhead;
                                while (fgets(buffer, 100, buy) != NULL)
                                {
                                    sscanf(buffer, "%d %d %d %d", &userIdTemp, &itemIdTemp, &priceTemp, &quantityTemp);
                                    buyList->itemId = itemIdTemp;
                                    buyList->quantity = quantityTemp;
                                    buyList->price = priceTemp;
                                    buyList->userId = userIdTemp;
                                    buyList->next = (struct buyQueue *)malloc(sizeof(struct buyQueue));
                                    buyList = buyList->next;
                                }
                                buyList->next = NULL;
                                fclose(buy);
                                buyList = buyhead;
                                while (buyList->next != NULL)
                                {
                                    if (buyList->userId == userId || buyList->price < price)
                                        buyList = buyList->next;
                                    else
                                    {
                                        if (buyList->quantity > quantity)
                                        {
                                            buyList->quantity = buyList->quantity - quantity;
                                            fprintf(trade, "%d %d %d %d %d\n", userId, buyList->userId, itemId, buyList->price, quantity);
                                            break;
                                        }
                                        else
                                        {
                                            fprintf(trade, "%d %d %d %d %d\n", userId, buyList->userId, itemId, buyList->price, buyList->quantity);
                                            if (buyList->quantity == quantity)
                                            {
                                                tempBuy = buyList;
                                                buyList = tempBuy->next;
                                                free(tempBuy);
                                                quantity = 0;
                                                break;
                                            }
                                            else
                                            {
                                                quantity -= buyList->quantity;
                                                buyList = buyList->next;
                                            }
                                        }
                                    }
                                }
                                if (quantity > 0)
                                    fprintf(sell, "%d %d %d %d\n", userId, itemId, price, quantity);
                                FILE *buyW = fopen("buy.txt", "w");
                                while (buyhead->next != NULL)
                                {
                                    fprintf(buyW, "%d %d %d %d\n", buyhead->userId, buyhead->itemId, buyhead->price, buyhead->quantity);
                                    // printf("%d %d %d %d\n", buyhead->userId, buyhead->itemId, buyhead->price, buyhead->quantity);
                                    tempBuy = buyhead;
                                    buyhead = tempBuy->next;
                                    free(tempBuy);
                                }
                                free(buyList);
                                free(buyhead);
                                fclose(buyW);
                                fclose(sell);
                                fclose(trade);
                            }
                            else if (textchecker == 2) // BUY
                            {
                                sscanf(buf, "%*d %d %d %d ", &itemId, &price, &quantity);
                                printf("\nBuy requested of %d no of item %d at %d by trader %d\n", quantity, itemId, price, userId);
                                FILE *buy = fopen("buy.txt", "a"),
                                     *sell = fopen("sell.txt", "r"),
                                     *trade = fopen("trades.txt", "a");
                                sellhead = (struct sellQueue *)malloc(sizeof(struct sellQueue));
                                tempSell = NULL;
                                sellList = sellhead;
                                while (fgets(buffer, 100, sell) != NULL)
                                {
                                    sscanf(buffer, "%d %d %d %d", &userIdTemp, &itemIdTemp, &priceTemp, &quantityTemp);
                                    printf("\n NEE 1 - %d %d %d %d\n", userIdTemp, itemIdTemp, priceTemp, quantityTemp);
                                    sellList->itemId = itemIdTemp;
                                    printf("\n1\n");
                                    sellList->quantity = quantityTemp;
                                    printf("\n2\n");
                                    sellList->price = priceTemp;
                                    printf("\n3\n");
                                    sellList->userId = userIdTemp;
                                    printf("\n4\n");
                                    sellList->next = (struct sellQueue *)malloc(sizeof(struct sellQueue));
                                    printf("\n5\n");
                                    sellList = sellList->next;
                                    printf("\n6\n");
                                }
                                sellList->next = NULL;
                                fclose(sell);
                                sellList = sellhead;
                                while (sellList->next != NULL)
                                {
                                    if (sellList->userId == userId || sellList->price > price)
                                        sellList = sellList->next;
                                    else
                                    {
                                        if (sellList->quantity > quantity)
                                        {
                                            sellList->quantity = sellList->quantity - quantity;
                                            fprintf(trade, "%d %d %d %d %d\n", sellList->userId, userId, itemId, sellList->price, quantity);
                                            break;
                                        }
                                        else
                                        {
                                            fprintf(trade, "%d %d %d %d %d\n", sellList->userId, userId, itemId, sellList->price, sellList->quantity);
                                            if (sellList->quantity == quantity)
                                            {
                                                tempSell = sellList;
                                                sellList = tempSell->next;
                                                free(tempSell);
                                                quantity = 0;
                                                break;
                                            }
                                            else
                                            {
                                                quantity -= sellList->quantity;
                                                sellList = sellList->next;
                                            }
                                        }
                                    }
                                }
                                if (quantity > 0)
                                    fprintf(buy, "%d %d %d %d\n", userId, itemId, price, quantity);
                                FILE *sellW = fopen("sell.txt", "w");
                                while (sellhead->next != NULL)
                                {
                                    printf("\n NEE 2 - %d %d %d %d\n", sellhead->userId, sellhead->itemId, sellhead->price, sellhead->quantity);
                                    fprintf(sellW, "%d %d %d %d\n", sellhead->userId, sellhead->itemId, sellhead->price, sellhead->quantity);
                                    tempSell = sellhead;
                                    sellhead = tempSell->next;
                                    free(tempSell);
                                }
                                free(sellList);
                                free(sellhead);
                                fclose(buy);
                                fclose(sellW);
                                fclose(trade);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            close(newSd);
        }
    }
}
