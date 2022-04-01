#include "lzjh_compress.h"
#include "lzjh_decompress.h"
#include <stdio.h>

void main_menu();

void main_menu()
{
    puts("Input the number to choose the function\n1.Compress\n2.Decompress\n3.Quit");
    while (1)
    {
        printf("Your choice:");
        unsigned char choice = getchar();

        // clean the input buffer
        int ch;
        while ((ch = getchar()) != EOF && ch != '\n');

        if (choice == '1')
        {
            compress();
        }
        else if (choice == '2')
        {
            decompress();
        }
        else if (choice == '3')
        {
            break;
        }
        else
        {
            puts("Check the number you inputted");
        }
    }
}


int main()
{
    main_menu();

    return 0;
}
