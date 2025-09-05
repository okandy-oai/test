#include <stdio.h>
#include <string.h>

int main() {
    char buffer[8];  // only 8 bytes

    // Unsafe: does not check input length
    strcpy(buffer, "This string is way too long!");
    
    printf("Buffer contains: %s\n", buffer);
    return 0;
}
