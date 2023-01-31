#include <stddef.h>
struct dog
{
    int x;
    int y;
};

int main()
{
    
    return offsetof(struct dog, y);
}