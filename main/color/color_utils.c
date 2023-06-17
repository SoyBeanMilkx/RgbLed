#include "color_utils.h"
#include <math.h>

//将十六进制颜色值转换为RGB
int *get_color(int color) {
    //为res分配内存空间
    int *res = malloc(sizeof(int) * 3);
    //检查是否分配成功
    if (res == NULL) {
        return NULL;
    }
    //提取RGB分量
    *(res + 0) = (color & 0xff0000) >> 16; //R
    *(res + 1) = (color & 0xff00) >> 8;    //G
    *(res + 2) = (color & 0xff);           //B

    return res;
}

//将String转化为int
int str_to_int(char *str) {
    char flag = '+';//指示结果是否带符号
    int res = 0;

    if (*str == '-') {
        ++str;//指向下一个字符
        flag = '-';
    }

    while (*str >= '0' && *str <= '9')
        res = 10 * res + *str++ - '0';

    if (flag == '-')
        res = -res;

    return res;
}

//计算色差
double color_diff(int R1, int G1, int B1, int R2, int G2, int B2) {
    //使用欧氏距离公式
    return sqrt((R1 - R2) * (R1 - R2) + (G1 - G2) * (G1 - G2) + (B1 - B2) * (B1 - B2));
}