#include <stdio.h>

// Tail recursion with return value
int gcd(int a, int b)
{
    if (b == 0) {
        return a;
    }
    return gcd(b, a % b);
}

// No recursion
int gcd_iter(int a, int b)
{
    while (b != 0) {
        int t = a;
        a = b;
        b = t % b;
    }
    return a;
}

// Tail recursion with void as return type
void print_arr(int arr[], int n)
{
    if (n == 0) {
        printf("\n");
        return;
    }
    printf("%d ", arr[0]);
    print_arr(arr + 1, n - 1);
}

// No recursion
void print_arr_iter(int arr[], int n)
{
    for (int i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

// Not a tail recursion but can be transformed into one with an accumulator
int factrorial(int n)
{
    if (n <= 0)
        return 1;
    return n * factrorial(n - 1);
}

// Tail recursion
int factrorial_acc(int n, int acc)
{
    if (n <= 0)
        return acc;
    return factrorial_acc(n - 1, n * acc);
}

// No recursion
int factrorial_iter(int n)
{
    int acc = 1;
    for (int i = 2; i <= n; i++) {
        acc *= i;
    }
    return acc;
}

int main()
{
    printf("GCD(12, 18) = %d = %d\n", gcd(12, 18), gcd_iter(12, 18));
    int arr[] = {1, 2, 3, 4};
    print_arr(arr, 4);
    print_arr_iter(arr, 4);
    printf("5! = %d = %d = %d\n", factrorial(5), factrorial_acc(5, 1), factrorial_iter(5));
    return 0;
}
