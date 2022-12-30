#define TEST_FUNC(x) #x
int printf(char* s, ...);
int main()
{
    const char* s = TEST_FUNC(hello world);
    printf("%s", s);
}