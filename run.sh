gcc main.c -o main && ./main
gcc -g buffer.c -o main && ./main

gcc test_uhash.c -o test_uhash && ./test_uhash

gcc -g main.c -o main && gdb ./main

gcc unic.c -I /opt/homebrew/opt/libunistring/include/ -L /opt/homebrew/opt/libunistring/lib -lunistring -o main && ./main


gcc hello_libvterm.c  -I /opt/homebrew/opt/libvterm/include/ -L /opt/homebrew/opt/libvterm/lib -lvterm -o hello_libvterm

gcc test_shell.c -o test_shell && ./test_shell
