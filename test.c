
struct dog
{
    int x;
    int y;
};

int main()
{
    
    return &((struct dog*)0x00)->y;
}