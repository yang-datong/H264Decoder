#ifndef __CONTEXTMODEL3DBUFFER__
#define __CONTEXTMODEL3DBUFFER__

#include <memory.h>
#include <stdio.h>

#include "CommonDef.h"
#include "ContextModel.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// context model 3D buffer class
class ContextModel3DBuffer {
 protected:
  ContextModel *m_contextModel; ///< array of context models
  const UInt m_sizeX;           ///< X size of 3D buffer
  const UInt m_sizeXY;          ///< X times Y size of 3D buffer
  const UInt m_sizeXYZ;         ///< total size of 3D buffer

 public:
  ContextModel3DBuffer(UInt uiSizeZ, UInt uiSizeY, UInt uiSizeX,
                       ContextModel *basePtr, Int &count);
  ~ContextModel3DBuffer() {}

  // access functions
  ContextModel &get(UInt uiZ, UInt uiY, UInt uiX) {
    return m_contextModel[uiZ * m_sizeXY + uiY * m_sizeX + uiX];
  }
  ContextModel *get(UInt uiZ, UInt uiY) {
    return &m_contextModel[uiZ * m_sizeXY + uiY * m_sizeX];
  }
  ContextModel *get(UInt uiZ) { return &m_contextModel[uiZ * m_sizeXY]; }

  // initialization & copy functions
  Void initBuffer(SliceType eSliceType, Int iQp,
                  UChar *ctxModel); ///< initialize 3D buffer by slice type & QP

  UInt calcCost(
      SliceType sliceType, Int qp,
      UChar *
          ctxModel); ///< determine cost of choosing a probability table based on current probabilities
  /** copy from another buffer
   * \param src buffer to copy from
   */
  Void copyFrom(const ContextModel3DBuffer *src) {
    assert(m_sizeXYZ == src->m_sizeXYZ);
    //::memcpy( m_contextModel, src->m_contextModel, sizeof(ContextModel) * m_sizeXYZ );

    for (size_t i = 0; i < m_sizeXYZ; ++i) {
      m_contextModel[i] = src->m_contextModel[i]; // 使用拷贝赋值运算符
    }
  }
};

//! \}

#endif // _HM_CONTEXT_MODEL_3DBUFFER_H_
