#include "PPS.hpp"
#include "BitStream.hpp"
#include <iostream>

using namespace std;

int PPS::extractParameters(BitStream &bs, uint32_t chroma_format_idc,
                           SPS spss[MAX_SPS_COUNT]) {
  pic_parameter_set_id = bs.readUE();
  seq_parameter_set_id = bs.readUE();
  m_sps = &spss[seq_parameter_set_id];
  cout << "\tPPS ID:" << pic_parameter_set_id
       << ",SPS ID:" << seq_parameter_set_id << endl;
  dependent_slice_segments_enabled_flag = bs.readUn(1);
  cout << "\t是否允许依赖片段分割:" << dependent_slice_segments_enabled_flag
       << endl;
  output_flag_present_flag = bs.readUn(1);
  cout << "\t输出标志的存在性:" << output_flag_present_flag << endl;
  num_extra_slice_header_bits = bs.readUn(3);
  cout << "\t片头额外的位数:" << num_extra_slice_header_bits << endl;
  sign_data_hiding_enabled_flag = bs.readUn(1);
  cout << "\t数据隐藏的启用标志，用于控制编码噪声:"
       << sign_data_hiding_enabled_flag << endl;
  cabac_init_present_flag = bs.readUn(1);
  cout << "\t是否为每个切片指定CABAC初始化参数:" << cabac_init_present_flag
       << endl;
  num_ref_idx_l0_default_active = bs.readUE() + 1;
  num_ref_idx_l1_default_active = bs.readUE() + 1;
  cout << "\t默认的L0和L1参考图像索引数量:" << num_ref_idx_l0_default_active
       << "," << num_ref_idx_l1_default_active << endl;
  init_qp = bs.readSE() + 26;
  cout << "\t初始量化参数:" << init_qp << endl;
  constrained_intra_pred_flag = bs.readUn(1);
  cout << "\t限制内部预测的启用标志:" << constrained_intra_pred_flag << endl;
  transform_skip_enabled_flag = bs.readUn(1);
  cout << "\t转换跳过的启用标志:" << transform_skip_enabled_flag << endl;
  cu_qp_delta_enabled_flag = bs.readUn(1);
  cout << "\t控制单元(CU)QP差分的启用标志:" << cu_qp_delta_enabled_flag << endl;
  if (cu_qp_delta_enabled_flag) {
    diff_cu_qp_delta_depth = bs.readUE();
    cout << "\tCU的QP差分深度:" << diff_cu_qp_delta_depth << endl;
    Log2MinCuQpDeltaSize = m_sps->CtbLog2SizeY - diff_cu_qp_delta_depth;
  }
  pps_cb_qp_offset = bs.readSE();
  pps_cr_qp_offset = bs.readSE();
  cout << "\t色度QP偏移:" << pps_cb_qp_offset << "," << pps_cr_qp_offset
       << endl;
  pps_slice_chroma_qp_offsets_present_flag = bs.readUn(1);
  cout << "\t切片层面的色度QP偏移是否存在:"
       << pps_slice_chroma_qp_offsets_present_flag << endl;
  weighted_pred_flag = bs.readUn(1);
  weighted_bipred_flag = bs.readUn(1);
  cout << "\t加权预测和双向加权预测的启用标志:" << weighted_pred_flag << ","
       << weighted_bipred_flag << endl;
  transquant_bypass_enabled_flag = bs.readUn(1);
  cout << "\t量化和变换绕过的启用标志:" << transquant_bypass_enabled_flag
       << endl;
  tiles_enabled_flag = bs.readUn(1);
  cout << "\t瓦片的启用标志:" << tiles_enabled_flag << endl;
  entropy_coding_sync_enabled_flag = bs.readUn(1);
  cout << "\t熵编码同步的启用标志:" << entropy_coding_sync_enabled_flag << endl;
  if (tiles_enabled_flag) {
    num_tile_columns = bs.readUE() + 1;
    num_tile_rows = bs.readUE() + 1;
    cout << "\t瓦片列数和行数:" << num_tile_columns << "," << num_tile_rows
         << endl;
    uniform_spacing_flag = bs.readUn(1);
    cout << "\t瓦片是否均匀分布:" << uniform_spacing_flag << endl;
    if (!uniform_spacing_flag) {
      column_width_minus1[32] = {0};
      row_height_minus1[32] = {0};
      for (int32_t i = 0; i < num_tile_columns - 1; i++)
        column_width_minus1[i] = bs.readUE();
      for (int32_t i = 0; i < num_tile_rows - 1; i++)
        row_height_minus1[i] = bs.readUE();
      //cout << "\t定义瓦片的列宽和行高减1:" << column_width_minus1, row_height_minus1 << endl;
    }
    loop_filter_across_tiles_enabled_flag = bs.readUn(1);
    cout << "\t是否允许循环滤波器跨瓦片工作:"
         << loop_filter_across_tiles_enabled_flag << endl;
  }
  pps_loop_filter_across_slices_enabled_flag = bs.readUn(1);
  cout << "\t是否允许循环滤波器跨切片工作:"
       << pps_loop_filter_across_slices_enabled_flag << endl;
  deblocking_filter_control_present_flag = bs.readUn(1);
  if (deblocking_filter_control_present_flag) {
    deblocking_filter_override_enabled_flag = bs.readUn(1);
    cout << "\t解块滤波器覆盖的启用标志:"
         << deblocking_filter_override_enabled_flag << endl;
    pps_deblocking_filter_disabled_flag = bs.readUn(1);
    cout << "\t解块滤波器禁用的标志:" << pps_deblocking_filter_disabled_flag
         << endl;
    if (!pps_deblocking_filter_disabled_flag) {
      pps_beta_offset_div2 = bs.readSE();
      pps_tc_offset_div2 = bs.readSE();
      cout << "\t解块滤波器的β偏移和tc偏移除以2:" << pps_beta_offset_div2 << ","
           << pps_tc_offset_div2 << endl;
    }
  }
  pps_scaling_list_data_present_flag = bs.readUn(1);
  cout << "\tPPS是否包含量化缩放列表:" << pps_scaling_list_data_present_flag
       << endl;
  if (pps_scaling_list_data_present_flag) scaling_list_data(bs);
  lists_modification_present_flag = bs.readUn(1);
  cout << "\t是否允许修改参考列表:" << lists_modification_present_flag << endl;
  Log2ParMrgLevel = log2_parallel_merge_level = bs.readUE() + 2;
  cout << "\t并行合并级别的对数值:" << log2_parallel_merge_level << endl;
  slice_segment_header_extension_present_flag = bs.readUn(1);
  cout << "\t切片段头扩展的存在标志:"
       << slice_segment_header_extension_present_flag << endl;
  pps_extension_present_flag = bs.readUn(1);
  cout << "\tPPS扩展的存在标志:" << pps_extension_present_flag << endl;
  pps_range_extension_flag = false;
  pps_multilayer_extension_flag = false;
  pps_3d_extension_flag = false;
  pps_scc_extension_flag = false;
  cout << "\t各种PPS扩展的启用标志:" << pps_range_extension_flag << ","
       << pps_multilayer_extension_flag << "," << pps_3d_extension_flag << ","
       << pps_scc_extension_flag << endl;
  pps_extension_4bits = false;
  cout << "\tPPS扩展数据是否存在:" << pps_extension_4bits << endl;
  if (pps_extension_present_flag) {
    pps_range_extension_flag = bs.readUn(1);
    pps_multilayer_extension_flag = bs.readUn(1);
    pps_3d_extension_flag = bs.readUn(1);
    pps_scc_extension_flag = bs.readUn(1);
    pps_extension_4bits = bs.readUn(4);
  }
  // 7.4.3.3.2 Picture parameter set range extension semantics
  if (pps_range_extension_flag) pps_range_extension(bs);
  //if (pps_multilayer_extension_flag) pps_multilayer_extension();
  //if (pps_3d_extension_flag) pps_3d_extension();
  if (pps_scc_extension_flag) pps_scc_extension(bs);
  if (pps_extension_4bits) {
    while (bs.more_rbsp_data())
      bs.readUn(1);
  }
  bs.rbsp_trailing_bits();

  //------------
  //
  /* ---- */

  int rowBd[32] = {0};
  int colBd[32] = {0};
  int i, j = 0;

  if (uniform_spacing_flag)
    for (j = 0; j < num_tile_rows; j++)
      rowHeight[j] = ((j + 1) * m_sps->PicHeightInCtbsY) / (num_tile_rows) -
                     (j * m_sps->PicHeightInCtbsY) / (num_tile_rows);
  else {
    rowHeight[num_tile_rows - 1] = m_sps->PicHeightInCtbsY;
    for (j = 0; j < num_tile_rows - 1; j++) {
      rowHeight[j] = row_height_minus1[j] + 1;
      rowHeight[num_tile_rows - 1] -= rowHeight[j];
    }
  }

  if (uniform_spacing_flag)
    for (i = 0; i < num_tile_columns; i++)
      colWidth[i] = ((i + 1) * m_sps->PicWidthInCtbsY) / (num_tile_columns) -
                    (i * m_sps->PicWidthInCtbsY) / (num_tile_columns);
  else {
    colWidth[num_tile_columns - 1] = m_sps->PicWidthInCtbsY;
    for (i = 0; i < num_tile_columns - 1; i++) {
      colWidth[i] = column_width_minus1[i] + 1;
      colWidth[num_tile_columns - 1] -= colWidth[i];
    }
  }

  for (rowBd[0] = 0, j = 0; j < num_tile_rows; j++)
    rowBd[j + 1] = rowBd[j] + rowHeight[j];
  for (colBd[0] = 0, i = 0; i < num_tile_columns; i++)
    colBd[i + 1] = colBd[i] + colWidth[i];

  for (i = 0, j = 0; i < m_sps->ctb_width; i++) {
    if (i > colBd[j]) j++;
    col_idxX[i] = j;
  }

  /* ------ */
  CtbAddrRsToTs = new uint8_t[m_sps->PicSizeInCtbsY]{0};
  /* TODO YangJing tileX,tileY 初始值？ <24-10-23 08:15:59> */
  int tbX, tbY, tileX = 0, tileY = 0;
  for (int ctbAddrRs = 0; ctbAddrRs < m_sps->PicSizeInCtbsY; ctbAddrRs++) {
    tbX = ctbAddrRs % m_sps->PicWidthInCtbsY;
    tbY = ctbAddrRs / m_sps->PicWidthInCtbsY;
    for (int i = 0; i < num_tile_columns; i++)
      if (tbX >= colBd[i]) tileX = i;
    for (int j = 0; j < num_tile_rows; j++)
      if (tbY >= rowBd[j]) tileY = j;
    CtbAddrRsToTs[ctbAddrRs] = 0;
    for (int i = 0; i < tileX; i++)
      CtbAddrRsToTs[ctbAddrRs] += rowHeight[tileY] * colWidth[i];
    for (int j = 0; j < tileY; j++)
      CtbAddrRsToTs[ctbAddrRs] += m_sps->PicWidthInCtbsY * rowHeight[j];
    CtbAddrRsToTs[ctbAddrRs] +=
        (tbY - rowBd[tileY]) * colWidth[tileX] + tbX - colBd[tileX];
  }

  CtbAddrTsToRs = new uint8_t[m_sps->PicSizeInCtbsY]{0};
  for (int ctbAddrRs = 0; ctbAddrRs < m_sps->PicSizeInCtbsY; ctbAddrRs++)
    CtbAddrTsToRs[CtbAddrRsToTs[ctbAddrRs]] = ctbAddrRs;

  for (int j = 0, tileIdx = 0; j <= num_tile_rows; j++)
    for (int i = 0; i < num_tile_columns; i++, tileIdx++)
      for (int y = rowBd[j]; y < rowBd[j + 1]; y++)
        for (int x = colBd[i]; x < colBd[i + 1]; x++)
          TileId[CtbAddrRsToTs[y * m_sps->PicWidthInCtbsY + x]] = tileIdx;

  return 0;
}

int PPS::scaling_list_data(BitStream &bs) {
  for (int sizeId = 0; sizeId < 4; sizeId++)
    for (int matrixId = 0; matrixId < 6; matrixId += (sizeId == 3) ? 3 : 1) {
      scaling_list_pred_mode_flag[sizeId][matrixId] = bs.readUn(1);
      if (!scaling_list_pred_mode_flag[sizeId][matrixId])
        scaling_list_pred_matrix_id_delta[sizeId][matrixId] = bs.readUE();
      else {
        int nextCoef = 8;
        int coefNum = MIN(64, (1 << (4 + (sizeId << 1))));
        if (sizeId > 1) {
          scaling_list_dc_coef_minus8[sizeId - 2][matrixId] = bs.readSE();
          nextCoef = scaling_list_dc_coef_minus8[sizeId - 2][matrixId] + 8;
        }
        for (int i = 0; i < coefNum; i++) {
          int scaling_list_delta_coef = bs.readSE();
          nextCoef = (nextCoef + scaling_list_delta_coef + 256) % 256;
          ScalingList[sizeId][matrixId][i] = nextCoef;
        }
      }
    }
  return 0;
}

int PPS::pps_range_extension(BitStream &bs) {
  if (transform_skip_enabled_flag)
    log2_max_transform_skip_block_size_minus2 = bs.readUE();
  cross_component_prediction_enabled_flag = bs.readUn(1);
  chroma_qp_offset_list_enabled_flag = bs.readUn(1);
  if (chroma_qp_offset_list_enabled_flag) {
    diff_cu_chroma_qp_offset_depth = bs.readUE();
    int chroma_qp_offset_list_len_minus1 = bs.readUE();
    for (int i = 0; i <= chroma_qp_offset_list_len_minus1; i++) {
      cb_qp_offset_list[i] = bs.readSE();
      cr_qp_offset_list[i] = bs.readSE();
    }
  }
  log2_sao_offset_scale_luma = bs.readUE();
  log2_sao_offset_scale_chroma = bs.readUE();
  return 0;
}

int PPS::pps_scc_extension(BitStream &bs) {
  pps_curr_pic_ref_enabled_flag = bs.readUn(1);
  residual_adaptive_colour_transform_enabled_flag = bs.readUn(1);
  if (residual_adaptive_colour_transform_enabled_flag) {
    pps_slice_act_qp_offsets_present_flag = bs.readUn(1);
    pps_act_y_qp_offset_plus5 = bs.readSE();
    pps_act_cb_qp_offset_plus5 = bs.readSE();
    pps_act_cr_qp_offset_plus3 = bs.readSE();
  }
  pps_palette_predictor_initializers_present_flag = bs.readUn(1);
  if (pps_palette_predictor_initializers_present_flag) {
    pps_num_palette_predictor_initializers = bs.readUE();
    if (pps_num_palette_predictor_initializers > 0) {
      monochrome_palette_flag = bs.readUn(1);
      luma_bit_depth_entry_minus8 = bs.readUE();
      if (!monochrome_palette_flag) chroma_bit_depth_entry_minus8 = bs.readUE();
      int numComps = monochrome_palette_flag ? 1 : 3;
      for (int comp = 0; comp < numComps; comp++) {
        int bit_depth = comp == 0 ? luma_bit_depth_entry_minus8 + 8
                                  : chroma_bit_depth_entry_minus8 + 8;
        for (int i = 0; i < pps_num_palette_predictor_initializers; i++)
          pps_palette_predictor_initializer[comp][i] = bs.readUn(bit_depth);
      }
    }
  }
  return 0;
}
