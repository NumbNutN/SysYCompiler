int get[6];
int get3[5][5] = {{},{1,2}};

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
    int get2[5][5] = {{},{1,2}};
    getstr(get);
    int lengets = getstr(get2[2]);
    int i = 0;
    while(i < lengets)
    {
        putch(get[i]);
        ++i;
    }
}