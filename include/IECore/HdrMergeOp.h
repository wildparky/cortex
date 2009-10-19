//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2009, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#ifndef IE_CORE_HDRMERGEOP_H
#define IE_CORE_HDRMERGEOP_H

#include "IECore/Op.h"
#include "IECore/SimpleTypedParameter.h"
#include "IECore/NumericParameter.h"

namespace IECore
{

IE_CORE_FORWARDDECLARE( ObjectParameter )

/// The HdrMergeOp merges a set of images with different exposures into a single HDR image.
//\todo Take in consideration Alpha channel from input images.
class HdrMergeOp : public Op
{
	public :

		IE_CORE_DECLARERUNTIMETYPED( HdrMergeOp, Op );

		HdrMergeOp();
		virtual ~HdrMergeOp();

		/// The Parameter for the group with input images.
		ObjectParameterPtr inputGroupParameter();
		ConstObjectParameterPtr inputGroupParameter() const;

		FloatParameterPtr exposureStepParameter();
		ConstFloatParameterPtr exposureStepParameter() const;

		FloatParameterPtr exposureAdjustmentParameter();
		ConstFloatParameterPtr exposureAdjustmentParameter() const;

		Box2fParameterPtr windowingParameter();
		ConstBox2fParameterPtr windowingParameter() const;

	protected :

		virtual ObjectPtr doOperation( ConstCompoundObjectPtr operands );

	private :

		ObjectParameterPtr m_inputGroupParameter;
		FloatParameterPtr m_exposureStepParameter;
		FloatParameterPtr m_exposureAdjustmentParameter;
		Box2fParameterPtr m_windowingParameter;

};

IE_CORE_DECLAREPTR( HdrMergeOp );

} // namespace IECore

#endif // IE_CORE_HDRMERGEOP_H
