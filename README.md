# Project_UnixProgramming
2018학년도 2학기 서울시립대학교 컴퓨터과학부 유닉스프로그래밍및실습 텀프로젝트 수행 내용을 공개합니다. 유닉스 환경에서 간단한 쉘을 제공합니다.

## Example of Use

```
msh # ls -al
total 120
drwxr-xr-x@  4 dewy  staff    128 Dec 12 00:52 .
drwxr-xr-x@ 10 dewy  staff    320 Dec 12 00:52 ..
-rwxr-xr-x   1 dewy  staff  51048 Dec 12 00:52 mini_sh
-rw-r--r--   1 dewy  staff   7266 Dec 12 00:52 mini_sh.c
```

```
msh # ls > a.txt 
msh # cat < a.txt
a.txt
mini_sh
mini_sh.c
```

```
msh # ls -al | grep txt
-rw-r--r--   1 dewy  staff     24 Dec 12 00:58 a.txt
```
