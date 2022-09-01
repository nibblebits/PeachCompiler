
union abc
{
    int x;
    int y;
};

union abc a;
int main()
{
    a.x = 50;
    return a.y;
}