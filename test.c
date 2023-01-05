#define TEST(a, b) a ## b
int printf(const char* fmt, int a);
int main()
{   
    printf("%i", TEST(50, 20));
}