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

    void *memS = (void *)0;
    struct sellQueue *sellList;
    srand((unsigned int)getpid());
    int shmSellId = shmget((key_t)SELL_KEY, sizeof(struct sellQueue), 0666 | IPC_CREAT);
    memS = shmat(shmSellId, (void *)0, 0);
    sellList = (struct sellQueue *)memS;
    // sellList = NULL;
    sellList->itemId = 0;
    sellList->quantity = 0;
    sellList->price = 0;
    sellList->userId = 0;
    sellList->next = NULL;

    // Buy List
    void *memB = (void *)0;
    struct buyQueue *buyList;
    srand((unsigned int)getpid());
    int shmBuyId = shmget((key_t)BUY_KEY, sizeof(struct buyQueue), 0666 | IPC_CREAT);
    memB = shmat(shmBuyId, (void *)0, 0);
    buyList = (struct buyQueue *)memB;
    // buyList = NULL;
    buyList->itemId = 0;
    buyList->quantity = 0;
    buyList->price = 0;
    buyList->userId = 0;
    buyList->next = NULL;

    // Trade List
    void *memTr = (void *)0;
    struct tradeQueue *tradeList;
    srand((unsigned int)getpid());
    int shmTradeId = shmget((key_t)TRADE_KEY, sizeof(struct tradeQueue), 0666 | IPC_CREAT);
    memTr = shmat(shmTradeId, (void *)0, 0);
    tradeList = (struct tradeQueue *)memTr;
    tradeList = NULL;
    // tradeList->itemId = 0;
    // tradeList->quantity = 0;
    // tradeList->price = 0;
    // tradeList->userId = 0;
    // tradeList->next = NULL;

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
                                int quantity, price, itemId;
                                sscanf(buf, "%*d %d %d %d ", &itemId, &price, &quantity);
                                printf("\nSale requested of %d no of item %d at %d\n", quantity, itemId, price);
                                // struct buyQueue *buyData = buyList;
                                while (1)
                                {
                                    struct buyQueue *buyData = buyList;
                                    printf("\nEnters in 0.1");
                                    if (buyData->next == NULL)
                                    {
                                        printf("\nEnters in 1");
                                        struct sellQueue *sellData = sellList;
                                        j = 1;
                                        while (sellData->next != NULL)
                                        {
                                            sellData = sellData->next;
                                            j++;
                                        }
                                        int shmSIId = shmget((key_t)(SELL_KEY + j), sizeof(struct sellQueue), 0666 | IPC_CREAT);
                                        sellData->next = (struct sellQueue *)shmat(shmSIId, 0, 0);
                                        sellData->next->itemId = itemId;
                                        sellData->next->quantity = quantity;
                                        sellData->next->price = price;
                                        sellData->next->userId = userId;
                                        sellData->next->next = NULL;
                                        items[itemId - 1].quantity += quantity;
                                        if (items[itemId - 1].bestSell == 0 || items[itemId - 1].bestSell > price)
                                            items[itemId - 1].bestSell = price;
                                        break;
                                    }
                                    else if (buyData->next->userId == userId || buyData->next->itemId != itemId || buyData->next->price < price)
                                    {
                                        printf("\nEnters in 2");
                                        buyData = buyData->next;
                                        continue;
                                    }
                                    else
                                    {
                                        printf("\nEnters in 3");

                                        int buyQuantity;
                                        if (buyData->next->quantity == quantity)
                                        {
                                            printf("\nEnters in 4");
                                            struct buyQueue *head = buyData;
                                            struct buyQueue *freeItem = buyData->next;
                                            head->next = freeItem->next;
                                            free(freeItem);
                                            buyQuantity = quantity;
                                        }
                                        else if (buyData->next->quantity > quantity)
                                        {
                                            printf("\nEnters in 5");
                                            buyQuantity = buyData->next->quantity - quantity;
                                            buyData->next->quantity = buyQuantity;
                                        }
                                        else
                                        {

                                            printf("\nEnters in 6");

                                            buyQuantity = quantity - buyData->next->quantity;
                                            struct sellQueue *sellData = sellList;
                                            j = 1;
                                            while (sellData->next != NULL)
                                            {
                                                sellData = sellData->next;
                                                j++;
                                            }
                                            int shmTrIId = shmget((key_t)(SELL_KEY + j), sizeof(struct sellQueue), 0666 | IPC_CREAT);
                                            sellData->next = (struct sellQueue *)shmat(shmTrIId, 0, 0);
                                            sellData->next->itemId = itemId;
                                            sellData->next->quantity = buyQuantity;
                                            sellData->next->price = price;
                                            sellData->next->userId = userId;
                                            sellData->next->next = NULL;

                                            items[itemId - i].quantity += quantity;
                                        }
                                        struct tradeQueue *tradeData = tradeList;
                                        j = 1;
                                        while (tradeData != NULL)
                                        {
                                            tradeData = tradeData->next;
                                            j++;
                                        }
                                        printf("\nEnters in 7");

                                        tradeData->itemId = itemId;
                                        tradeData->quantity = buyQuantity;
                                        tradeData->price = price;
                                        tradeData->customerId = buyData->next->userId;
                                        tradeData->sellerId = userId;
                                        int shmTrIId = shmget((key_t)(TRADE_KEY + j), sizeof(struct tradeQueue), 0666 | IPC_CREAT);
                                        tradeData->next = (struct tradeQueue *)shmat(shmTrIId, 0, 0);
                                        tradeData->next = NULL;

                                        if (items[itemId - 1].bestSell == 0 || items[itemId - 1].bestSell > price)
                                            items[itemId - 1].bestSell = price;
                                        break;
                                    }
                                }
                                struct sellQueue *sellTrav = sellList;
                                struct buyQueue *buyTrav = buyList;
                                struct tradeQueue *tradeTrav = tradeList;
                                while (sellTrav->next != NULL)
                                {
                                    printf("\nItem Sell %d -> quantity: %d, price: %d, trader: %d\n", sellTrav->next->itemId, sellTrav->next->quantity, sellTrav->next->price, sellTrav->next->userId);
                                    sellTrav = sellTrav->next;
                                }
                                while (buyTrav->next != NULL)
                                {
                                    printf("\nItem Buy %d -> quantity: %d, price: %d, trader: %d\n", buyTrav->next->itemId, buyTrav->next->quantity, buyTrav->next->price, buyTrav->next->userId);
                                    buyTrav = buyTrav->next;
                                }
                            }
                            else if (textchecker == 2) // BUY
                            {
                                int quantity, price, itemId;
                                sscanf(buf, "%*s %d %d %d ", &itemId, &price, &quantity);
                                printf("\nBuy requested of %d no of item %d at %d\n", quantity, itemId, price);
                                // struct sellQueue *sellData = sellList;
                                while (1)
                                {
                                    struct sellQueue *sellData = sellList;
                                    if (sellData->next == NULL)
                                    {
                                        struct buyQueue *buyData = buyList;
                                        j = 1;
                                        while (buyData->next != NULL)
                                        {
                                            buyData = buyData->next;
                                            j++;
                                        }
                                        int shmTrIId = shmget((key_t)(BUY_KEY + j), sizeof(struct buyQueue), 0666 | IPC_CREAT);
                                        buyData->next = (struct buyQueue *)shmat(shmTrIId, 0, 0);
                                        buyData->next->itemId = itemId;
                                        buyData->next->quantity = quantity;
                                        buyData->next->price = price;
                                        buyData->next->userId = userId;
                                        buyData->next->next = NULL;

                                        if (items[itemId - 1].bestBuy == 0 || items[itemId - 1].bestBuy < price)
                                            items[itemId - 1].bestBuy = price;
                                        break;
                                    }
                                    else if (sellData->next->userId == userId || sellData->next->itemId != itemId || sellData->next->price > price)
                                    {
                                        sellData = sellData->next;
                                        continue;
                                    }
                                    else
                                    {
                                        int sellQuantity;

                                        if (sellData->next->quantity == quantity)
                                        {
                                            struct sellQueue *head = sellData;
                                            struct sellQueue *freeItem = sellData->next;
                                            head->next = freeItem->next;
                                            free(freeItem);
                                            sellQuantity = quantity;
                                            items[itemId - 1].quantity -= quantity;
                                        }
                                        else if (sellData->next->quantity > quantity)
                                        {
                                            sellQuantity = sellData->next->quantity - quantity;
                                            sellData->next->quantity = sellQuantity;
                                            items[itemId - 1].quantity -= quantity;
                                        }
                                        else
                                        {
                                            sellQuantity = quantity - sellData->next->quantity;
                                            struct buyQueue *buyData = buyList;
                                            j = 1;
                                            while (buyData->next != NULL)
                                            {
                                                buyData = buyData->next;
                                                j++;
                                            }
                                            int shmTrIId = shmget((key_t)(BUY_KEY + j), sizeof(struct buyQueue), 0666 | IPC_CREAT);
                                            buyData->next = (struct buyQueue *)shmat(shmTrIId, 0, 0);
                                            buyData->next->itemId = itemId;
                                            buyData->next->quantity = sellQuantity;
                                            buyData->next->price = price;
                                            buyData->next->userId = userId;
                                            buyData->next->next = NULL;
                                            items[itemId - 1].quantity -= sellData->next->quantity;
                                        }
                                        struct tradeQueue *tradeData = tradeList;
                                        j = 1;
                                        while (tradeData != NULL)
                                        {
                                            tradeData = tradeData->next;
                                            j++;
                                        }
                                        tradeData->itemId = itemId;
                                        tradeData->quantity = sellQuantity;
                                        tradeData->price = price;
                                        tradeData->customerId = userId;
                                        tradeData->sellerId = sellData->next->userId;
                                        int shmTrIId = shmget((key_t)(TRADE_KEY + j), sizeof(struct tradeQueue), 0666 | IPC_CREAT);
                                        tradeData->next = (struct tradeQueue *)shmat(shmTrIId, 0, 0);
                                        tradeData->next = NULL;

                                        if (items[itemId - 1].bestBuy == 0 || items[itemId - 1].bestBuy < price)
                                            items[itemId - 1].bestBuy = price;
                                        break;
                                    }
                                }
                                struct sellQueue *sellTrav = sellList;
                                struct buyQueue *buyTrav = buyList;
                                struct tradeQueue *tradeTrav = tradeList;
                                while (sellTrav->next != NULL)
                                {
                                    printf("\nItem Sell %d -> quantity: %d, price: %d, trader: %d\n", sellTrav->next->itemId, sellTrav->next->quantity, sellTrav->next->price, sellTrav->next->userId);
                                    sellTrav = sellTrav->next;
                                }
                                while (buyTrav->next != NULL)
                                {
                                    printf("\nItem Buy %d -> quantity: %d, price: %d, trader: %d\n", buyTrav->next->itemId, buyTrav->next->quantity, buyTrav->next->price, buyTrav->next->userId);
                                    buyTrav = buyTrav->next;
                                }
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
