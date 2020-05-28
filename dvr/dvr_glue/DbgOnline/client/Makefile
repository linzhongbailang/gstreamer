all:
	gcc -I../inc debug_online_client.cpp -o dbg_client_x86
	/opt/arm-toolchain/linux/linaro/gcc-linaro-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-g++ -I../inc debug_online_client.cpp -o dbg_client_arm	
clean:
	rm -f *.o dbg_client_x86 dbg_client_arm
