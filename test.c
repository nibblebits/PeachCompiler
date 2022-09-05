int printf(const char* s, ...);
int main()
{
    int res;
    res = 0;
    if (1)
    {
        int m;
        m = 1;
        switch (m)
        {

        case 1:
            switch(5)
            {
                case 1:
                case 6:
                    res = 5;
                break;
                case 5:
                case 7:
                    res = 10;
                break;
            }
            break;

        case 4:
            res = 4;
            break;
        case 2:
            res = 2;
            break;
        }
    }

    switch(4)
    {
        case 2:

        break;

        default:
            res = res + 7;
    }
    printf("%i\n", res);
}   