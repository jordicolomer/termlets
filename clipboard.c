#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int clipboard_copy_apple(const char *text)
{
    if (text == NULL)
        return 0;

    FILE *pipe = popen("pbcopy", "w");
    if (pipe == NULL)
        return 0;

    size_t len = 0;
    while (text[len] != '\0')
        len++;

    size_t written = fwrite(text, 1, len, pipe);

    int status = pclose(pipe);

    return (written == len && status == 0);
}



void clipboard_copy(const char *text)
{
#ifdef _WIN32
    // Win32 implementation
#elif __APPLE__
    // pbcopy or Cocoa
    clipboard_copy_apple(text);
#elif __linux__
    // xclip/xsel/wl-copy
#else
    // unsupported
#endif
}

char *clipboard_paste_apple(void)
{
    FILE *pipe = popen("pbpaste", "r");
    if (pipe == NULL)
        return NULL;

    size_t capacity = 4096;
    size_t length = 0;

    char *buffer = malloc(capacity);
    if (buffer == NULL) {
        pclose(pipe);
        return NULL;
    }

    char temp[1024];

    while (1) {
        size_t n = fread(temp, 1, sizeof(temp), pipe);

        if (n > 0) {
            if (length + n + 1 > capacity) {
                capacity = (length + n + 1) * 2;

                char *new_buffer = realloc(buffer, capacity);
                if (new_buffer == NULL) {
                    free(buffer);
                    pclose(pipe);
                    return NULL;
                }

                buffer = new_buffer;
            }

            memcpy(buffer + length, temp, n);
            length += n;
        }

        if (n < sizeof(temp))
            break;
    }

    buffer[length] = '\0';

    int status = pclose(pipe);

    if (status != 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

char *clipboard_paste(void)
{
#ifdef _WIN32
    // Win32 implementation
    return NULL;
#elif __APPLE__
    return clipboard_paste_apple();
#elif __linux__
    // xclip/xsel/wl-paste implementation
    return NULL;
#else
    return NULL;
#endif
}
/*int main(){
    clipboard_copy("test");
}*/
