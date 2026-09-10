这些是ACMOJ上面的BASIC模块对应的所有测试点
你可以在本地环境运行以确保你的MiniVim输出和我们期望的答案相同
我们的ans均来自Vim的输出
比如说  ./vtemu/bin/vtemu -l 24 -c 80 vim < ./test/basic/1.in > ./test/basic/1.ans
该指令生成了1.in测试点对应的1.ans文件
你只需要把指令里面的vim换成你的MiniVim 
即 ./vtemu/bin/vtemu -l 24 -c 80 ./MiniVim < ./test/basic/1.in > ./test/basic/1.out
如果最后有这一行unable to read pty: Input/output error, 没关系
若 diff ./test/basic/1.out ./test/basic/1.ans 没有输出,则你的答案正确