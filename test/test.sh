#!/bin/bash

declare -A  ALL_MD5

#第一次成功解码后的输出文件MD5

demo_10_frames_h264(){
	index=11
	ALL_MD5['output_I_0.bmp']=14708835fa62b30c2b337deb9ca23649
	ALL_MD5['output_P_1.bmp']=18eadb2bfb6e4638a3b00d3aef71266f
	ALL_MD5['output_B_2.bmp']=8133475fd7fc44f1c64ebaf0dbe2e183
	ALL_MD5['output_B_3.bmp']=7a9d805a3e4f64343671262d676c2714
	ALL_MD5['output_B_4.bmp']=8b2666e71a6fd52e356e18bbd2e70609
	ALL_MD5['output_P_5.bmp']=cc7a55f1c502d173c0adaecebca0323b
	ALL_MD5['output_B_6.bmp']=3fa63ff2db87644e76ac28ff76b6956e
	ALL_MD5['output_P_7.bmp']=79f19718c2e71fea525ccad412e80b9b
	ALL_MD5['output_B_8.bmp']=ab9a2eb73d8bbfd6057e25ea091e7e12
	ALL_MD5['output_P_9.bmp']=9d01acc7f12aa62073a8596b2f79747c
	ALL_MD5['output_B_10.bmp']=391abc0f9c384605eb7b133087728148
}

demo_10_frames_interlace_h264(){
	echo -e "\033[31m不支持\033[0m"
}

demo_10_frames_cavlc_h264(){
	index=11
	ALL_MD5['output_I_0.bmp']=bece5ba2be612511f200769b1731ada6
	ALL_MD5['output_P_1.bmp']=09792bf3c1e6275891b7a8fd1c33434d
	ALL_MD5['output_B_2.bmp']=931a46fb1b080a53ef43d647095b4a5d
	ALL_MD5['output_B_3.bmp']=0f54e6d4525e3a4cefc56afaf484b48c
	ALL_MD5['output_B_4.bmp']=e2e3f9da46c28bf44300a08bb2dfe379
	ALL_MD5['output_P_5.bmp']=67064a9f13acc3b67e929876a115e037
	ALL_MD5['output_B_6.bmp']=107d0d1f0cb942e267130ce17488bd31
	ALL_MD5['output_P_7.bmp']=7e08e5fa6bf7e2397cd995471e22c94c
	ALL_MD5['output_B_8.bmp']=92db42a5d4d5ebe5275df552831479e9
	ALL_MD5['output_P_9.bmp']=d3e517b42cc25ee035ac71fa0d31c41d
	ALL_MD5['output_B_10.bmp']=955c29533decc52a6656c4e593a4fe38
}


demo_10_frames_cavlc_and_interlace_h264(){
	index=11
	ALL_MD5['output_I_0.bmp']=052be97c86acde1b1938b22ca2aeac46
	ALL_MD5['output_P_1.bmp']=de0e6a202710c0e47b2ab8aedf85dfef
	ALL_MD5['output_P_2.bmp']=eff652585d19bf9c883485311b298452
	ALL_MD5['output_P_3.bmp']=4961e029f7cead706478ba9b60619a0e
	ALL_MD5['output_P_4.bmp']=3f98e47d2df3367184042f749279a7e8
	ALL_MD5['output_P_5.bmp']=d0dcdd9a59395bf2aa9f6898bbd5f027
	ALL_MD5['output_P_6.bmp']=6073d878a3c63de18118df659331eba8
	ALL_MD5['output_P_7.bmp']=d813d07c42f7d7df3b998b307e293c32
	ALL_MD5['output_P_8.bmp']=0d5f7035ec4e72593c6bd142a1e1e713
	ALL_MD5['output_P_9.bmp']=7a9f35046a8f2f1ed2aa23cde68407e2
	ALL_MD5['output_P_10.bmp']=f81aa56faae3d167d832f089bb44e8ed
}

source_cut_10_frames_h264(){
	index=9
	ALL_MD5['output_I_0.bmp']=08d314916c72caa09e813b43f1744acb
	ALL_MD5['output_P_1.bmp']=e1484970d133814bc3bf3b57ab6cd38d
	ALL_MD5['output_B_2.bmp']=da76145e1ac64642fce3e894b3305768
	ALL_MD5['output_P_3.bmp']=cb64a877bf973895c15300aca827307d
	ALL_MD5['output_B_4.bmp']=5082ac48879db9e703c945297ea09f6b
	ALL_MD5['output_P_5.bmp']=d4b1cb390a8a3e021494a7d91ddd7388
	ALL_MD5['output_B_6.bmp']=a8db64316dfcdcb2df63408916c6342e
	ALL_MD5['output_B_7.bmp']=313134ec104972481fc4d0264ed91b5b
	ALL_MD5['output_P_8.bmp']=b7e55f6d5399a9624d21a0c24e34f658
}


source_cut_10_frames_no_B_h264(){
	index=9
	ALL_MD5['output_I_0.bmp']=74e36a035b8d75b31a02278450b42447
	ALL_MD5['output_P_1.bmp']=d5a671c3df0c3600783f590962c77ed3
	ALL_MD5['output_P_2.bmp']=82b44ef5ffe29fa4400284f49baf5120
	ALL_MD5['output_P_3.bmp']=f14afa86821a79635f72821809954858 #e2721d4ada9293326a453f790709b820
	ALL_MD5['output_P_4.bmp']=15d38fca08b5ed0d5adea2562f39e3bc #432b32e64d62525aba0a1447a020ede9
	ALL_MD5['output_P_5.bmp']=2a3adcfb06119568384542cda4d315fe #2dcb311b2a1d6f6b1eafc7912788ee5e
	ALL_MD5['output_P_6.bmp']=a9de600d6f657894d0e895cb4589d56a #b201424c985c0ab2a598d0ea796bdda6
	ALL_MD5['output_P_7.bmp']=2c66cc47676e77ced192d90447995845 #d32d5a3172b47d51d90877b9bd355f5c
	ALL_MD5['output_P_8.bmp']=16793d288834b9edbf20b34beaeb9da8 #41a3278a459c9eae41d3bbb5ee139800
}


main(){
	if [ ${index} != $(ls *.bmp | wc -l) ];then
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

	if [ ${index} != $count ];then
		echo "Output file md5 is changed , count:${count}/11"
		return -1
	fi

	echo "Test success!"
}

if [ "$1" == "demo_10_frames.h264" ];then
	demo_10_frames_h264
elif [ "$1" == "demo_10_frames_interlace.h264" ];then
	demo_10_frames_interlace_h264
elif [ "$1" == "demo_10_frames_cavlc.h264" ];then
	demo_10_frames_cavlc_h264
elif [ "$1" == "demo_10_frames_cavlc_and_interlace.h264" ];then
	demo_10_frames_cavlc_and_interlace_h264
elif [ "$1" == "source_cut_10_frames.h264" ];then
	source_cut_10_frames_h264
elif [ "$1" == "source_cut_10_frames_no_B.h264" ];then
	source_cut_10_frames_no_B_h264
else
	echo -e "\033[31mInputFile ???\033[0m";exit
fi

main $@
