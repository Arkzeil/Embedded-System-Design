5.1 Please explain what is arm-linux-gnueabihf-gcc? Why don’t we just compile with gcc? (5%)  
  
A:  
	If we use gcc to compile a source code, it would output assembly file that's specific to host CPU binary(i.e. can only run on the same architecture as your computer).  
	On the other hand, if we use cross compiler(e.g. arm-linux-gnueabihf-gcc), it would then generate targeted CPU binary(like ARM or MIPS architecture).

5.2 Can executable hello_world run on the host computer? Why or Why not?(5%)  
  
A:  
	No, since we used cross compiler to compiler our C code, the output assembly file is specific to the ARM CPU architecture. Our computer is x86 architecture, so there's no way that the hello_world can run on our computer.

ref:  
	https://ryan0988.pixnet.net/blog/post/171244470   
	https://blog.csdn.net/weixin_39328406/article/details/117202335   
	https://en.wikipedia.org/wiki/Cross_compiler