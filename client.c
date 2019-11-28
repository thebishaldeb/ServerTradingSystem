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

#define MAX_MSG 100

#define MEM_SZ 4096

int main(int argc, char *argv[])
{
    int i;
    int sd, rc, len;

    int choice, quantity, price, itemId, sellId, buyId, userId;

    struct sockaddr_in localAddr, servAddr;
    char buf[MAX_MSG], buffer[MAX_MSG];

    FILE *buy = fopen("buy.txt", "a"),
         *sell,
         *trade = fopen("trades.txt", "a");
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
    scanf("%d", &userId);
    sprintf(buf, "%d", userId);
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
            while (1)
            {
                printf("\nChoose a number among the following options: ");
                printf("\n\t1 - The status of the items");
                printf("\n\t2 - Sell an item");
                printf("\n\t3 - Buy an item");
                printf("\n\t4 - Status of your trades");
                printf("\n\t5 - Exit");
                printf("\n\nPlease select your choice: ");
                scanf("%d", &choice);
                switch (choice)
                {
                case 1:
                    printf("\nEnter the id of the trade items between 1, 2 and 3:");
                    scanf("%d", &choice);
                    buy = fopen("buy.txt", "r");
                    sell = fopen("sell.txt", "r");
                    printf("\nBuy requests:");
                    printf("\nBuyer\tItem\tQuantity\tPrice per item");
                    while (fgets(buffer, 100, buy) != NULL)
                    {
                        sscanf(buffer, "%d %d %d %d\n", &buyId, &itemId, &price, &quantity);
                        if (choice == itemId)
                            printf("\n%d\tItem %d\t%d\t\t%d", buyId, itemId, quantity, price);
                    }
                    printf("\n");
                    fclose(buy);
                    printf("\nSale Requests:");
                    printf("\nSeller\tItem\tQuantity\tPrice per item");
                    while (fgets(buffer, 100, sell) != NULL)
                    {
                        sscanf(buffer, "%d %d %d %d\n", &sellId, &itemId, &price, &quantity);
                        if (choice == itemId)
                            printf("\n%d\tItem %d\t%d\t\t%d", sellId, itemId, quantity, price);
                    }
                    printf("\n");
                    fclose(sell);
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
                    if (len = read(sd, buf, sizeof(buf)))
                        printf("%s\n", buf);
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
                    printf("\nRequest sent\n");
                    if (len = read(sd, buf, sizeof(buf)))
                        printf("%s\n", buf);
                    break;
                case 4:
                    trade = fopen("trades.txt", "r");
                    printf("\nYour trade status:");
                    printf("\nSeller\tBuyer\tItem\tQuantity\tPrice per item");
                    while (fgets(buffer, 100, trade) != NULL)
                    {
                        sscanf(buffer, "%d %d %d %d %d\n", &sellId, &buyId, &itemId, &price, &quantity);
                        if (sellId == userId || buyId == userId)
                            printf("\n%d\t%d\tItem %d\t%d\t\t%d", sellId, buyId, itemId, quantity, price);
                    }
                    printf("\n");
                    fclose(trade);
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
