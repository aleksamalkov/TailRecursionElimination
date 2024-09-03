#include <stdio.h>
#include <assert.h>

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

// Two tail recursive calls
int binary_search(int arr[], int start, int end, int x)
{
    if (end < start)
        return -1;

    int mid = start + (end - start) / 2;
    if (arr[mid] < x)
        return binary_search(arr, mid + 1, end, x);
    if (arr[mid] > x)
        return binary_search(arr, start, mid - 1, x);
    return mid;
}

// No recursion
int partition(int arr[], int start, int end)
{
    int p = start;
    for (int i = start + 1; i <= end; i++) {
        if (arr[i] < arr[start]) {
            p++;
            int t = arr[i];
            arr[i] = arr[p];
            arr[p] = t;
        }
    }
    int t = arr[start];
    arr[start] = arr[p];
    arr[p] = t;
    return p;
}

// Two recursive calls of which one is a tail call
void quick_sort(int arr[], int start, int end)
{
    if (end <= start)
        return;

    int p = partition(arr, start, end);
    quick_sort(arr, start, p);
    quick_sort(arr, p + 1, end);
}

int main()
{
    printf("GCD(12, 18) = %d = %d\n", gcd(12, 18), gcd_iter(12, 18));
    assert(gcd(12, 18) == gcd_iter(12, 18));
    assert(gcd(12, 18) == 6);

    int arr[] = {1, 2, 3, 4};
    printf("1 2 3 4 \n");
    print_arr(arr, 4);
    print_arr_iter(arr, 4);

    printf("5! = %d = %d = %d\n", factrorial(5), factrorial_acc(5, 1), factrorial_iter(5));
    assert(factrorial(5) == 120);
    assert(factrorial_acc(5, 1) == 120);
    assert(factrorial_iter(5) == 120);

    const int n = 7;
    int arr_sort[] = {5, 2, 7, 5, 4, 1, 3};
    print_arr(arr_sort, n);
    quick_sort(arr_sort, 0, n - 1);
    print_arr(arr_sort, n);

    printf("pos of 2: %d\n", binary_search(arr_sort, 0, n - 1, 2));
    assert(binary_search(arr_sort, 0, n - 1, 2) == 1);

    printf("pos of 6: %d\n", binary_search(arr_sort, 0, n - 1, 6));
    assert(binary_search(arr_sort, 0, n - 1, 6) == -1);
    
    return 0;
}
