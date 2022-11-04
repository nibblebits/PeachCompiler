#define TEST_FUNC(s) #s

int printf(const char *s, ...);
int main()
{
    const char *a = TEST_FUNC(hello);
    printf("%s\n", a);
}