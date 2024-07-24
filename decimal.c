#include <stdio.h>
#include <math.h>

typedef struct{
    unsigned int bits[4];
} s21_decimal;

void print_binary(int num) {
    for (int i = 31; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
    printf("\n");
}

void print_decimal_ten(s21_decimal *num) {
    for(int i = 3; i >= 0; i--) {
        printf("%d\n", num->bits[i]);
    }
}

void print_decimal(s21_decimal* num) {
    for(int i = 3; i >= 0; i--) {
        print_binary(num->bits[i]);
    }
}

void set_sign(s21_decimal* num, int sign) {
    num->bits[3] |= sign << 31;
}

void set_scale(s21_decimal* num, int scale) {
    num->bits[3] &= ~(0xFF << 16);
    num->bits[3] |= (scale << 16);
}

int get_scale(s21_decimal* num) {
    int result = 0;
    for(int i = 0; i < 8; i++) {
        if(num->bits[3] >> (16 + i) & 1) {
            result += pow(2, i);
        }
    } 
    return result;
}

int get_sign(s21_decimal* num) {
    return (num->bits[3] >> 31);
}

void align_scales(s21_decimal* num1, s21_decimal* num2) {
    int scale1 = (num1->bits[3] >> 16) & 0xFF;
    int scale2 = (num2->bits[3] >> 16) & 0xFF;
    
    if (scale1 > scale2) {
        while (scale1 > scale2) {
            scale2++;
            long temp = ((long)num2->bits[0]) * 10;
            num2->bits[0] = (int)temp;
            temp = ((long)num2->bits[1]) * 10 + (temp >> 32);
            num2->bits[1] = (int)temp;
            temp = ((long)num2->bits[2]) * 10 + (temp >> 32);
            num2->bits[2] = (int)temp;
        }
    } else if (scale2 > scale1) {
        while (scale2 > scale1) {
            scale1++;
            long temp = ((long)num1->bits[0]) * 10;
            num1->bits[0] = (int)temp;
            temp = ((long)num1->bits[1]) * 10 + (temp >> 32);
            num1->bits[1] = (int)temp;
            temp = ((long)num1->bits[2]) * 10 + (temp >> 32);
            num1->bits[2] = (int)temp;
        }
    }
}

void s21_add(s21_decimal* value1, s21_decimal* value2, s21_decimal* result) {
    s21_decimal temp1 = *value1;
    s21_decimal temp2 = *value2;

    align_scales(&temp1, &temp2);

    int buffer = 0;
    for (int i = 0; i < 3; i++) {
        long sum = (long)temp1.bits[i] + temp2.bits[i] + buffer;
        result->bits[i] = (int)sum;
        buffer = (sum >> 32);
    }
    result->bits[3] = temp1.bits[3];
}

void s21_sub(s21_decimal* value1, s21_decimal* value2, s21_decimal* result) {
    s21_decimal temp1 = *value1;
    s21_decimal temp2 = *value2;

    align_scales(&temp1, &temp2);

    int buffer = 0;
    for (int i = 0; i < 3; i++) {
        long diff = (long)temp1.bits[i] - temp2.bits[i] - buffer;
        if (diff < 0) {
            diff += ((long)1 << 32);
            buffer = 1;
        } else {
            buffer = 0;
        }
        result->bits[i] = (int)diff;
    }
    result->bits[3] = temp1.bits[3]; // Сохраняем знак и масштаб
    if (buffer) {
        for (int i = 0; i < 3; i++) {
            result->bits[i] = ~result->bits[i];
        }
        int carry = 1;
        for (int i = 0; i < 3; i++) {
            long temp = (long)result->bits[i] + carry;
            result->bits[i] = (int)temp;
            carry = temp >> 32;
        }
        result->bits[3] ^= 1 << 31; // Инвертируем знак
    }
}

void remove_trailing_zeros(s21_decimal* dec) {
    int scale = get_scale(dec);

    for (int i = 0; i < scale; i++) {
        // Проверяем остаток от деления на 10
        long remainder = ((long)dec->bits[2] % 10) * ((long)1 << 32) +
                             ((long)dec->bits[1] % 10) * ((long)1 << 32) +
                             (long)dec->bits[0] % 10;
        if (remainder != 0) break;

        // Делим число на 10
        long carry = 0;
        for (int j = 2; j >= 0; j--) {
            long value = ((long)dec->bits[j]) + (carry << 32);
            dec->bits[j] = (int)(value / 10);
            carry = value % 10;
        }
    }
}

void s21_mul(s21_decimal* value1, s21_decimal* value2, s21_decimal *result) {
    s21_decimal temp1 = *value1;
    s21_decimal temp2 = *value2;
    s21_decimal temp_result = {{0, 0, 0, 0}};

    int total_scale = get_scale(&temp1) + get_scale(&temp2);
    set_scale(&temp1, 0);
    set_scale(&temp2, 0);

    for(int i = 0; i < 3; i++) {
        for(unsigned int j = 0; j < temp2.bits[i]; j++) {
            if(!j) printf("%d\n", temp2.bits[i]);
            s21_add(&temp_result, &temp1, &temp_result);
        }
        if (i < 2) {
            long carry = 0;
            for (int k = 0; k < 3; k++) {
                long new_val = ((long)temp1.bits[k] << 32) | carry;
                temp1.bits[k] = (int)new_val;
                carry = new_val >> 32;
            }
        }
    }

    set_scale(&temp_result, total_scale);

    if (get_sign(&temp1) != get_sign(&temp2)) { set_sign(&temp_result, 1); } 
    else {set_sign(&temp_result, 0); }

    *result = temp_result;
}

int main() {
    s21_decimal value1 = {{12345, 0, 0, 2 << 16}}; // 123.45
    s21_decimal value2 = {{67890, 0, 0, 2 << 16}}; // 678.90

    set_sign(&value2, 1);
    s21_decimal result;
    // add_decimals(&result, &value1, &value2);

    // s21_add(&value1, &value2, &result);
    // printf("plus result:\n");
    // print_decimal_ten(&result);
    // print_decimal(&result); // Ожидаемый результат: 802.35

    // s21_sub(&value2, &value1, &result);
    // printf("\nminus result:\n");
    // print_decimal_ten(&result); 
    // print_decimal(&result); // Ожидаемый результат: 555.45

    s21_mul(&value1, &value2, &result);
    printf("\nMultiplication result:\n");
    print_decimal_ten(&result);
    print_decimal(&result); // Ожидаемый результат: 8376.21
    printf("%d\n", get_scale(&result));


    return 0;
}


// Multiplication result:
// -2147221504
// 650752620
// -1691956814
// 1822107213
// 10000000000001000000000000000000
// 00100110110010011011001001101100
// 10011011001001101100100110110010
// 01101100100110110010011001001101

// Multiplication result:
// -2147221504
// 650752620
// -1691956814
// 1822107213
// 10000000000001000000000000000000
// 00100110110010011011001001101100
// 10011011001001101100100110110010
// 01101100100110110010011001001101
