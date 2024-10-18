#include "GOP.hpp"
#include "Frame.hpp"

GOP::GOP() {
  for (int i = 0; i < MAX_DPB; i++) {
    m_dpb_for_output[i] = nullptr;
    m_dpb[i] = new Frame;
  }
}

GOP::~GOP() {
  for (int i = 0; i < MAX_DPB; i++) {
    free(m_dpb[i]);
    m_dpb[i] = nullptr;
  }
}

// lastDecodedPic: 上一个完成解码的帧
// outPic: 输出的已解码帧，或者不输出帧（即nullptr)
int GOP::outputOneFrame(Frame *lastDecodedPic, Frame *&outPic) {
  //无B Slice情况，直接输出该帧
  if (m_max_num_reorder_frames == 0) {
    outPic = lastDecodedPic;
    return 0;
  }

  //含B Slice情况，输出已解码且最小POC帧
  int index = -1;
  for (int i = 0; i < m_dpb_for_output_length; ++i)
    if (i == 0 || m_dpb_for_output[i]->m_picture_frame.PicOrderCnt <
                      m_dpb_for_output[index]->m_picture_frame.PicOrderCnt)
      index = i;

  if (lastDecodedPic != nullptr) {
    // 不满足最大重排序帧数，不输出帧（最大重排序帧数有B帧一般是2）
    if (m_dpb_for_output_length < m_max_num_reorder_frames) {
      m_dpb_for_output[m_dpb_for_output_length] = lastDecodedPic;
      m_dpb_for_output_length++;
      outPic = nullptr;
    }
    // 达到最大重排序帧数，输出帧POC最小的帧
    else {
      /* 上一解码帧比缓冲区的帧POC更小，直接输出它 */
      if (lastDecodedPic->m_picture_frame.PicOrderCnt <
          m_dpb_for_output[index]->m_picture_frame.PicOrderCnt)
        outPic = lastDecodedPic;
      /* 将缓冲区最小POC的帧输出后，该帧纳入缓冲区 */
      else
        outPic = m_dpb_for_output[index],
        m_dpb_for_output[index] = lastDecodedPic;
    }
  }
  // 当前正在解码帧为IDR帧，将整个缓冲区所有帧按POC最小开始全部输出
  else {
    // 存在未输出的帧，从DPB中的POC最小的帧开始输出
    if (m_dpb_for_output_length > 0) {
      if (index >= 0)
        outPic = m_dpb_for_output[index];
      else {
        outPic = nullptr;
        m_dpb_for_output_length = 0;
        return 0;
      }
      //从输出帧的索引处，队列往前移动
      for (int32_t i = index; i < m_dpb_for_output_length; ++i)
        m_dpb_for_output[i] = m_dpb_for_output[i + 1];
      m_dpb_for_output_length--;
    } else
      outPic = nullptr;
  }

  return 0;
}

int GOP::flush() {
  Frame *outPicture = nullptr;
  while (true) {
    //从gop保存的帧数组中一个个读取输出
    outputOneFrame(nullptr, outPicture);
    if (outPicture)
      outPicture->m_is_in_use = false;
    else
      break;
  }
  return 0;
}
