#!/bin/bash

#移动去块滤波器
declare -A  ALL_MD5

#第一次成功解码后的输出文件MD5

demo_10_frames_h264(){
	index=11
	ALL_MD5['output_I_0.bmp']=b2568511e534e8358020045ace0f9c87
	ALL_MD5['output_P_1.bmp']=d7b536c2192b5b71dba3c549ce40750d
	ALL_MD5['output_B_2.bmp']=1207e6bb2e2285e9f3182ae8ecfa2556
	ALL_MD5['output_B_3.bmp']=b9408790fff3ca106b4274c7e3a8dcc3
	ALL_MD5['output_B_4.bmp']=c31ec6af99cfc86006effad4ce5aadab
	ALL_MD5['output_P_5.bmp']=4cc9d089893916c5a9d7a5e415de86e5
	ALL_MD5['output_B_6.bmp']=768cac1e9dc1286f93b2187ffe04d96e
	ALL_MD5['output_P_7.bmp']=a9c59152113f9dd2736351e43561a11a
	ALL_MD5['output_B_8.bmp']=e5e1c89f0111bee9e527c5ba2501e875
	ALL_MD5['output_P_9.bmp']=43a8cde3fe965a1057621a468f9da62b
	ALL_MD5['output_B_10.bmp']=98d01d39cb57c77a6c9ca65e627a9696
}

demo_10_frames_interlace_h264(){
	index=11
	ALL_MD5['output_I_0.bmp']=5896fb59b3aa811856858e8dd6ca63a8
	ALL_MD5['output_P_1.bmp']=77c70f7e96889c269294f644b5bfa198
	ALL_MD5['output_B_2.bmp']=48a19343153696903905a3d6a2fc2cbf
	ALL_MD5['output_B_3.bmp']=f2541110d68ab2e2bbcc86cb62da68e7
	ALL_MD5['output_B_4.bmp']=174b186fa3c817f82d4cb14443c8bc66
	ALL_MD5['output_P_5.bmp']=27153a360bb3be7917c437c3adb204d8
	ALL_MD5['output_B_6.bmp']=b0f6cadc8a9ad8eecdbf747580c2b890
	ALL_MD5['output_P_7.bmp']=1aa857107f5aad21d910fca189fa0296
	ALL_MD5['output_B_8.bmp']=a0aab3c38a778f931e7c2bec3fca557a
	ALL_MD5['output_P_9.bmp']=74280fe55de5aaace0f39dfc671da4cf
	ALL_MD5['output_B_10.bmp']=5c96ef9e8a3d5d7bda105a2dd3404adc
}

demo_10_frames_cavlc_h264(){
	index=11
	ALL_MD5['output_I_0.bmp']=aa4030e6758b8cdabcb4a7278a31d0a9
	ALL_MD5['output_P_1.bmp']=ee7b9737cc6349da9927359b5a0e69d3
	ALL_MD5['output_B_2.bmp']=f3afa7727c7d614df2f429c7ad9489f0
	ALL_MD5['output_B_3.bmp']=38882221c78b0096cce04d108c51c61d
	ALL_MD5['output_B_4.bmp']=b754e395399f6781af3cf080a5ffed5d
	ALL_MD5['output_P_5.bmp']=e73fc727589a372f78fb60dd52cafd63
	ALL_MD5['output_B_6.bmp']=93b142e9c62cba0c7c4ee343ba7b1216
	ALL_MD5['output_P_7.bmp']=6ecb0590abbb0e25351405e79bd6d05e
	ALL_MD5['output_B_8.bmp']=7d120b1bae1a9c952820ccdf0a3b2c42
	ALL_MD5['output_P_9.bmp']=a35551c52256ad9d58c5e2a43df55f95
	ALL_MD5['output_B_10.bmp']=d3178adb5dad21c4dc1a9e487c9a61db
}


demo_10_frames_cavlc_and_interlace_h264(){
	index=11
	ALL_MD5['output_I_0.bmp']=9cbf2c8b5cf428c74a8d0a0981d45ae5
	ALL_MD5['output_P_1.bmp']=e8966fadaa3667fdfb8a38c79259d44a
	ALL_MD5['output_P_2.bmp']=88d0698aea9668a083baadb42cf1abf5
	ALL_MD5['output_P_3.bmp']=f8a765081f9d45a057ec773a21ad2e18
	ALL_MD5['output_P_4.bmp']=8e530662365152358a77ef90eefef567
	ALL_MD5['output_P_5.bmp']=ac2d470e128267dff8da90e3feeb0607
	ALL_MD5['output_P_6.bmp']=c14bae1faed247deb584f35e70aa7541
	ALL_MD5['output_P_7.bmp']=6373c65029d385bd37f6bbc7a18b9d37
	ALL_MD5['output_P_8.bmp']=6c2b4080ca7eafb7e872f64a25b33c0e
	ALL_MD5['output_P_9.bmp']=a604b79d3e0d332bff62515730e25f6e
	ALL_MD5['output_P_10.bmp']=0e09bacdda11bdec3d6342ab5a7c9823
}

source_cut_10_frames_h264(){
	index=9
	ALL_MD5['output_I_0.bmp']=0940885394bd5b9af249968f955c9761
	ALL_MD5['output_P_1.bmp']=8908411ea2e2d66f077ae2ae71d66135
	ALL_MD5['output_B_2.bmp']=018c25d8e40152a7c9088411ccfc45e0
	ALL_MD5['output_P_3.bmp']=b8bf917008877814ff4913c4159e7bce
	ALL_MD5['output_B_4.bmp']=f8a5e66441bf60950aa58c628176d22b
	ALL_MD5['output_P_5.bmp']=c0276ef258909a4d6d7cd53f315234c3
	ALL_MD5['output_B_6.bmp']=3e9b96ebba400ad76d5d8f10e7381820
	ALL_MD5['output_B_7.bmp']=014e5b7925eb4a5082c56976a454833c
	ALL_MD5['output_P_8.bmp']=786257fb0aa0c3bb4d2adf1dc9080636
}


source_cut_10_frames_no_B_h264(){
	index=9
	ALL_MD5['output_I_0.bmp']=0023add10ab462cd3d9eab94ab690673
	ALL_MD5['output_P_1.bmp']=be62b704ac4d57cf24e0a542bd1f7a4d
	ALL_MD5['output_P_2.bmp']=81144737c7cb6313e468c370cffff833
	ALL_MD5['output_P_3.bmp']=bdda8ac8e3118a8c2cfc468764ae4ff5
	ALL_MD5['output_P_4.bmp']=e962b44a20ccc9a2c18aba9c79c6757a
	ALL_MD5['output_P_5.bmp']=ffbd1785896dea50009a033af411a496
	ALL_MD5['output_P_6.bmp']=230a9c820827816ddc2643ac17ee2989
	ALL_MD5['output_P_7.bmp']=2009f1ec0937d1dd1dbca0f7e5285221
	ALL_MD5['output_P_8.bmp']=05ee597d672cc5370f49560496cc935e
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
