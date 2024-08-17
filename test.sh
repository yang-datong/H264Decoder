#!/bin/bash

declare -A  ALL_MD5

#第一次成功解码后的输出文件MD5
ALL_MD5['output_B_10.bmp']=391abc0f9c384605eb7b133087728148
ALL_MD5['output_B_2.bmp']=8133475fd7fc44f1c64ebaf0dbe2e183
ALL_MD5['output_B_3.bmp']=7a9d805a3e4f64343671262d676c2714
ALL_MD5['output_B_4.bmp']=8b2666e71a6fd52e356e18bbd2e70609
ALL_MD5['output_B_6.bmp']=3fa63ff2db87644e76ac28ff76b6956e
ALL_MD5['output_B_8.bmp']=ab9a2eb73d8bbfd6057e25ea091e7e12
ALL_MD5['output_I_0.bmp']=14708835fa62b30c2b337deb9ca23649
ALL_MD5['output_P_1.bmp']=18eadb2bfb6e4638a3b00d3aef71266f
ALL_MD5['output_P_5.bmp']=cc7a55f1c502d173c0adaecebca0323b
ALL_MD5['output_P_7.bmp']=79f19718c2e71fea525ccad412e80b9b
ALL_MD5['output_P_9.bmp']=9d01acc7f12aa62073a8596b2f79747c


main(){
	if [ 11 != $(ls *.bmp | wc -l) ];then
		echo "Output file is bad"
		return -1
	fi

	local count=0
	for i in *.bmp;do
		local current_md5=$(md5sum $i | awk '{print $1}')
		for key in "${!ALL_MD5[@]}"; do
			if [ "$current_md5" == "${ALL_MD5[$key]}" ];then
				((count++))
				echo $current_md5 [yes]
				break;
			fi
		done
	done

	if [ 11 != $count ];then
		echo "Output file md5 is changed , count:${count}/11"
		return -1
	fi

	echo "Test success!"
}

main $@
