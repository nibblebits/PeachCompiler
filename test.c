int printf(const char* s, ...);

struct dog
{
    int a;
    int b;
};

void set_dog(struct dog* d, int x)
{
    d->b = x;
}
int main()
{
    struct dog d;
    set_dog(&d, 70);
    printf("hello world %i\n", d.b);
}   