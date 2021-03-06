// -*- C++ -*-

// PLearn (A C++ Machine Learning Library)
// Copyright (C) 1998 Pascal Vincent
// Copyright (C) 1999-2002 Pascal Vincent, Yoshua Bengio, Rejean Ducharme and University of Montreal
// Copyright (C) 2001-2002 Nicolas Chapados, Ichiro Takeuchi, Jean-Sebastien Senecal
// Copyright (C) 2002 Xiangdong Wang, Christian Dorion

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  1. Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
// 
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
// 
//  3. The name of the authors may not be used to endorse or promote
//     products derived from this software without specific prior written
//     permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
// NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This file is part of the PLearn library. For more information on the PLearn
// library, go to the PLearn Web site at www.plearn.org


/* *******************************************************      
 * $Id$
 * This file is part of the PLearn library.
 ******************************************************* */

#ifndef AffineTransformWeightPenalty_INC
#define AffineTransformWeightPenalty_INC

#include "UnaryVariable.h"
#include <plearn/base/stringutils.h>

namespace PLearn {
using namespace std;


//! Weight decay terms for affine transforms
class AffineTransformWeightPenalty: public UnaryVariable
{
    typedef UnaryVariable inherited;

protected:    
    real weight_decay_;
    real bias_decay_;
    string penalty_type_;

public:
    //!  Default constructor for persistence
    AffineTransformWeightPenalty()
        : weight_decay_(0.0), bias_decay_(0.0), penalty_type_("L2_square")
    {
        string pt = lowerstring( penalty_type_ );
        if( pt == "l1" )
            penalty_type_ = "L1";
        else if( pt == "l1_square" || pt == "l1 square" || pt == "l1square" )
            penalty_type_ = "L1_square";
        else if( pt == "l2_square" || pt == "l2 square" || pt == "l2square" )
            penalty_type_ = "L2_square";
        else if( pt == "l2" )
        {
            PLWARNING("L2 penalty not supported, assuming you want L2 square");
            penalty_type_ = "L2_square";
        }
        else
            PLERROR("penalty_type_ \"%s\" not supported", penalty_type_.c_str());
    }

    AffineTransformWeightPenalty(Variable* affinetransform, real weight_decay, real bias_decay=0., string penalty_type="L2_square")
        : inherited(affinetransform, 1,1),weight_decay_(weight_decay),bias_decay_(bias_decay),penalty_type_(penalty_type)
    {
        string pt = lowerstring( penalty_type_ );
        if( pt == "l1" )
            penalty_type_ = "L1";
        else if( pt == "l1_square" || pt == "l1 square" || pt == "l1square" )
            penalty_type_ = "L1_square";
        else if( pt == "l2_square" || pt == "l2 square" || pt == "l2square" )
            penalty_type_ = "L2_square";
        else if( pt == "l2" )
        {
            PLWARNING("L2 penalty not supported, assuming you want L2 square");
            penalty_type_ = "L2_square";
        }
        else
            PLERROR("penalty_type_ \"%s\" not supported", penalty_type_.c_str());
    }

    PLEARN_DECLARE_OBJECT(AffineTransformWeightPenalty);
    static void declareOptions(OptionList &ol);

    virtual void recomputeSize(int& l, int& w) const;    
    virtual void fprop();
    virtual void bprop();
};

DECLARE_OBJECT_PTR(AffineTransformWeightPenalty);

//! weight decay and bias decay terms
//! This has not been tested yet [Pascal: a tester].
inline Var affine_transform_weight_penalty(Var transformation, real weight_decay, real bias_decay=0, string penalty_type="L2_square")
{ return new AffineTransformWeightPenalty(transformation, weight_decay, bias_decay, penalty_type); } 

} // end of namespace PLearn

#endif 


/*
  Local Variables:
  mode:c++
  c-basic-offset:4
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:79
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=79 :
