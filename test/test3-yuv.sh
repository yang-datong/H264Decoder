#!/bin/bash

#移动去块滤波器
declare -A ALL_MD5

#第一次成功解码后的输出文件MD5

demo_10_frames_h264() {
	index=1
	ALL_MD5['output.yuv']=683af3a830237c6e39c3ff5d49e1748b
}

demo_10_frames_interlace_h264() {
	index=1
	ALL_MD5['output.yuv']=xx
}

demo_10_frames_cavlc_h264() {
	index=1
	ALL_MD5['output.yuv']=d9661e3e712cca6f486c28ae0c7728f7
}

demo_10_frames_cavlc_and_interlace_h264() {
	index=1
	ALL_MD5['output.yuv']=281428e2e98337f1230f548ed5264013
}

source_cut_10_frames_h264() {
	index=1
	ALL_MD5['output.yuv']=fe91f8d428b5a59e10ed9ad81bd32152
}

source_cut_10_frames_no_B_h264() {
	index=1
	ALL_MD5['output.yuv']=b63113656a26ef2fa450c9daaac68768
}

main() {
	if [ ${index} != $(ls *.yuv | wc -l) ]; then
		echo "Output file is bad"
		return -1
	fi

	local count=0
	for i in *.yuv; do
		local current_md5=$(md5sum $i | awk '{print $1}')
		for key in "${!ALL_MD5[@]}"; do
			if [ "$current_md5" == "${ALL_MD5[$key]}" ]; then
				((count++))
				echo $current_md5 [yes]
				break
			fi
		done
	done

	if [ ${index} != $count ]; then
		echo "Output file md5 is changed , count:${count}/11"
		return -1
	fi

	echo "Test success!"
}

if [ "$1" == "demo_10_frames.h264" ]; then
	demo_10_frames_h264
elif [ "$1" == "demo_10_frames_interlace.h264" ]; then
	demo_10_frames_interlace_h264
elif [ "$1" == "demo_10_frames_cavlc.h264" ]; then
	demo_10_frames_cavlc_h264
elif [ "$1" == "demo_10_frames_cavlc_and_interlace.h264" ]; then
	demo_10_frames_cavlc_and_interlace_h264
elif [ "$1" == "source_cut_10_frames.h264" ]; then
	source_cut_10_frames_h264
elif [ "$1" == "source_cut_10_frames_no_B.h264" ]; then
	source_cut_10_frames_no_B_h264
else
	echo -e "\033[31mInputFile ???\033[0m"
	exit
fi

main $@
