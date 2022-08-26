struct dog
{
    int a;
    int b;
    int e;
    char kk;
};

struct dog get_dog()
{
    struct dog d;
    d.b = 50;
    return d;
}

struct dog a;
int main()
{
    a = get_dog();
    return a.b;
}