#include <stdio.h>
#include <string.h>
#include <stdint.h>

// print memory in hex + ASCII
void inspect_memory(const void *address, size_t size) {
    const unsigned char *p = (const unsigned char *)address;
    size_t i, j;

    // go through memory 16 bytes at a time
    for (i = 0; i < size; i += 16) {
        // print address at start of line
        printf("%p  ", (const void *)(p + i));

        // print hex part
        for (j = 0; j < 16; j++) {
            if (i + j < size) {
                printf("%02x ", p[i + j]);
            } else {
                printf("   "); // keep columns aligned
            }
        }

        // print ASCII part
        printf(" ");
        for (j = 0; j < 16; j++) {
            if (i + j < size) {
                unsigned char c = p[i + j];
                if (c >= 32 && c <= 126) {
                    printf("%c", c);
                } else {
                    printf(".");
                }
            }
        }
        printf("\n");
    }
}

struct Item {
    int id;
    double value;
    char tag;
};

int main(void) {
    struct Item item;
    item.id = 12345;
    item.value = -42.5;
    item.tag = 'Z';

    int arr[10] = {0, 1, 2, 3, -1, -2, 100, 256, 999, 42};
    char msg[] = "Hello CS5008!";

    printf("struct Item:\n");
    inspect_memory(&item, sizeof(item));
    printf("\n");

    printf("int array:\n");
    inspect_memory(arr, sizeof(arr));
    printf("\n");

    printf("string:\n");
    inspect_memory(msg, strlen(msg) + 1);
    printf("\n");

    return 0;
}
