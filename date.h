#pragma once

struct date {
    int year, month, day;
};

/* days can be negative, does not account for leap years */
struct date
dateadd(struct date x, int days)
{
    const int monthdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    struct date y = x;

    if (days > 0) {
        y.day = y.day + days % 365;
        for (; y.day > monthdays[y.month - 1]; y.day -= monthdays[y.month - 1], y.month = (y.month + 1) % 12);
        y.year += days / 365;
    } else {
        y.day += days;
        for (; y.day < 1; y.day += monthdays[y.month - 1]) {
            if (--y.month < 1) {
                y.year -= 1;
                y.month = 12;
            }
        }
    }

    return y;
}

char
datewithin(struct date low, struct date x, struct date high)
{
    if (x.year == low.year) {
        if (x.month < low.month)
            return 0;
        if (x.month == low.month && x.day < low.day)
            return 0;
    }

    if (x.year == high.year) {
        if (x.month > high.month)
            return 0;
        if (x.month == high.month && x.day > high.day)
            return 0;
    }

    return x.year >= low.year && x.year <= high.year;
}
