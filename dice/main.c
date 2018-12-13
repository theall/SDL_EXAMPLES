#include <stdio.h>
#include <time.h>
#include <windows.h>

#define BET_BIG      1
#define BET_SMALL    2
#define BET_DOUBLE   3
#define BET_TRIBBLE  4
#define BET_BASE     100

#define PRIZE_TIMES_BIG         2
#define PRIZE_TIMES_SMALL       2
#define PRIZE_TIMES_DOUBLE      4
#define PRIZE_TIMES_TRIBBLE     10

int getDiceNumber()
{
    return rand()%6+1;
}

int playDice()
{
    int i;
    printf("ҡ����");
    for(i=0; i<3; i++)
    {
        printf(".");
        Sleep(300);
    }
    int n = getDiceNumber();
    printf("%d\n", n);
    return n;
}

void display()
{
    printf(
        "**********************************\n"
        "*     ������Ϸ����������Ѻע     *\n"
        "*     1 ������%3d��            *\n"
        "*     2 С������%3d��            *\n"
        "*     3 ���ӣ�����%3d��          *\n"
        "*     4 ���ӣ�����%3d��          *\n"
        "*     8 ��ʾʣ����             *\n"
        "*     9 ��ʾ���˵�               *\n"
        "*     0 �˳�����                 *\n"
        "**********************************\n",
        PRIZE_TIMES_BIG,
        PRIZE_TIMES_SMALL,
        PRIZE_TIMES_DOUBLE,
        PRIZE_TIMES_TRIBBLE
    );
}

int main()
{
    int money = 500;
    srand(time(0));
    display();
    printf("�𲽽�� %d��\n", money);

    while(1)
    {
        printf(">");
        int input;
        int times = 1;
        scanf("%d", &input);

        int cost = times*BET_BASE;
        if(money-cost<0)
        {
            printf("���㣬���ֵ��\n");
            continue;
        }
        switch(input)
        {
        case 1:
            printf("��Ѻע���󡿣����-%d\n", cost);
            break;
        case 2:
            printf("��Ѻע��С�������-%d\n", cost);
            break;
        case 3:
            printf("��Ѻע�����ӡ������-%d\n", cost);
            break;
        case 4:
            printf("��Ѻע�����ӡ������-%d\n", cost);
            break;
        case 8:
            printf("ʣ��%d��\n", money);
            continue;
            break;
        case 9:
            display();
            continue;
        case 0:
            return 0;
            break;
        default:
            printf("�����������������!\n");
            continue;
            break;
        }
        money -= cost;

        int dice1 = playDice();
        int dice2 = playDice();
        int dice3 = playDice();
        int isBig = (dice1+dice2+dice3)>9;
        int isDouble = (dice1==dice2) || (dice1==dice3) || (dice3==dice2);
        int isTribble = (dice1==dice2) && (dice2==dice3);

        int bonus = 0;
        if(input<3)
        {
            if(isBig && input==BET_BIG)
                bonus = PRIZE_TIMES_BIG;
            else if (!isBig && input==BET_SMALL)
                bonus = PRIZE_TIMES_SMALL;
        }
        else if (input==BET_DOUBLE && isDouble)
        {
            bonus = PRIZE_TIMES_DOUBLE;
        }
        else if (input==BET_TRIBBLE && isTribble)
        {
            bonus = PRIZE_TIMES_TRIBBLE;
        }

        if(bonus > 0)
        {
            int prize = bonus * cost;
            money += prize;
            printf("�н�����������%d��,", prize);
        } else {
            printf("δ�н���");
        }
        printf("ʣ��%d��\n", money);
    }
    return 0;
}
