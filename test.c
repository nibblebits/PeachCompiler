#include <stdarg.h>

int sum(int num, ...)
{
    int result = 0;
    va_list list;
    va_start(list, num);
    int i = 0;
    for (i = 0; i < num; i+=1)
    {
        result += va_arg(list, int);
    }
    
    va_end(list);
    return result;
}

int main()
{
    return sum(3, 20, 30, 40);
}