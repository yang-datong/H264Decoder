/** \file     ContextModel.h
    \brief    context model class (header)
*/

#ifndef __CONTEXTMODEL__
#define __CONTEXTMODEL__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CommonDef.h"
#include "TComRom.h"

//! \ingroup TLibCommon
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// context model class
class ContextModel {
 public:
  ContextModel() {
    m_ucState = 0;
    m_binsCoded = 0;
  }
  ~ContextModel() {}

  UChar getState() { return (m_ucState >> 1); } ///< get current state
  UChar getMps() { return (m_ucState & 1); }    ///< get curret MPS
  Void setStateAndMps(UChar ucState, UChar ucMPS) {
    m_ucState = (ucState << 1) + ucMPS;
  } ///< set state and MPS

  Void init(Int qp,
            Int initValue); ///< initialize state with initial probability

  Void updateLPS() { m_ucState = m_aucNextStateLPS[m_ucState]; }

  Void updateMPS() { m_ucState = m_aucNextStateMPS[m_ucState]; }

  Int getEntropyBits(Short val) { return m_entropyBits[m_ucState ^ val]; }

#if FAST_BIT_EST
  Void update(Int binVal) { m_ucState = m_nextState[m_ucState][binVal]; }
  static Void buildNextStateTable();
  static Int getEntropyBitsTrm(Int val) { return m_entropyBits[126 ^ val]; }
#endif
  Void setBinsCoded(UInt val) { m_binsCoded = val; }
  UInt getBinsCoded() { return m_binsCoded; }

 private:
  UChar m_ucState; ///< internal state variable

  static const UInt m_totalStates =
      (1 << CONTEXT_STATE_BITS) * 2; //*2 for MPS = [0|1]
  static const UChar m_aucNextStateMPS[m_totalStates];
  static const UChar m_aucNextStateLPS[m_totalStates];
  static const Int m_entropyBits[m_totalStates];
#if FAST_BIT_EST
  static UChar m_nextState[m_totalStates][2 /*MPS = [0|1]*/];
#endif
  UInt m_binsCoded;
};

//! \}

#endif
