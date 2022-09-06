int printf(const char* s, ...);
int special(char c1, int i1)
{
   return c1+i1;
}

int main()
{
    printf("%i\n", special(22, 10));
}