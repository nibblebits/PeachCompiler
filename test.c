#include <stdio.h>

int main()
{
    FILE* f = fopen("./testing.txt", "w");
    fwrite("hello", 5, 1, f);
}