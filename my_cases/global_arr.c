int get[6] ;

int getstr(int get[]) {
    int x = getch();
    int length = 0;
    while (x != 13 && x != 10) {
        get[length] = x;
        length = length + 1;
        x = getch();
    }
    return length;
}

int main()
{
    int lengets = getstr(get);
    int i = 0;
    while(i < lengets)
    {
        putch(get[i]);
        ++i;
    }
}