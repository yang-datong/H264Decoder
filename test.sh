#!/bin/bash



if [ 11 != $(ls *.bmp | wc -l) ];then
	echo "Output file is bad"
	return -1
else
	echo "Test success!"
fi

#第一次成功解码后的输出文件MD5
#391abc0f9c384605eb7b133087728148  output_B_10.bmp
#8133475fd7fc44f1c64ebaf0dbe2e183  output_B_2.bmp
#7a9d805a3e4f64343671262d676c2714  output_B_3.bmp
#8b2666e71a6fd52e356e18bbd2e70609  output_B_4.bmp
#3fa63ff2db87644e76ac28ff76b6956e  output_B_6.bmp
#ab9a2eb73d8bbfd6057e25ea091e7e12  output_B_8.bmp
#14708835fa62b30c2b337deb9ca23649  output_I_0.bmp
#18eadb2bfb6e4638a3b00d3aef71266f  output_P_1.bmp
#cc7a55f1c502d173c0adaecebca0323b  output_P_5.bmp
#79f19718c2e71fea525ccad412e80b9b  output_P_7.bmp
#9d01acc7f12aa62073a8596b2f79747c  output_P_9.bmp
