// -*- C++ -*-

// PLearn (A C++ Machine Learning Library)
// Copyright (C) 1998 Pascal Vincent
// Copyright (C) 1999-2001 Pascal Vincent, Yoshua Bengio, Rejean Ducharme and University of Montreal
// Copyright (C) 2002 Pascal Vincent, Julien Keable, Xavier Saint-Mleux
//
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
   * $Id: ConcatRowsVMatrix.cc,v 1.11 2004/09/14 16:04:39 chrish42 Exp $
   ******************************************************* */

#include "ConcatRowsVMatrix.h"
#include "SelectColumnsVMatrix.h"

namespace PLearn {
using namespace std;

/** ConcatRowsVMatrix **/

PLEARN_IMPLEMENT_OBJECT(ConcatRowsVMatrix,
    "Concatenates the rows of a number of VMat.",
    "It can also be used to select fields which are common to those VMat,\n"
    "using the 'only_common_fields' option.\n"
    "Otherwise, the fields are just assumed to be those of the first VMat.\n"
);

///////////////////////
// ConcatRowsVMatrix //
///////////////////////
ConcatRowsVMatrix::ConcatRowsVMatrix(TVec<VMat> the_array)
: array(the_array),
  fully_check_mappings(false),
  only_common_fields(false)
{
  if (array.size() > 0)
    build_();
};

ConcatRowsVMatrix::ConcatRowsVMatrix(VMat d1, VMat d2)
: fully_check_mappings(false),
  only_common_fields(false)
{
  array.resize(2);
  array[0] = d1;
  array[1] = d2;
  build_();
};

////////////////////
// declareOptions //
////////////////////
void ConcatRowsVMatrix::declareOptions(OptionList &ol)
{
  declareOption(ol, "array", &ConcatRowsVMatrix::array, OptionBase::buildoption,
      "The VMat to concatenate.");

  declareOption(ol, "fully_check_mappings", &ConcatRowsVMatrix::fully_check_mappings, OptionBase::buildoption,
      "If set to 1, then columns for which there is a string <-> real mapping will be examined\n"
      "to ensure that no numerical data in a VMat conflicts with a mapping in another VMat.");

  declareOption(ol, "only_common_fields", &ConcatRowsVMatrix::only_common_fields, OptionBase::buildoption,
      "If set to 1, then only the fields whose names are common to all VMat\n"
      "in 'array' will be kept (and reordered if needed).");

  inherited::declareOptions(ol);
}

///////////
// build //
///////////
void ConcatRowsVMatrix::build()
{
  inherited::build();
  build_();
}

////////////
// build_ //
////////////
void ConcatRowsVMatrix::build_()
{
  int n = array.size();
  if (n < 1)
    PLERROR("ConcatRowsVMatrix expects underlying-array of length >= 1, got 0");

  // Get the fields info.
  fieldinfos = array[0]->getFieldInfos();
  if (only_common_fields) {
    findCommonFields();
  } else {
    to_concat = array;
  }

  // Set length_ and width_.
  recomputeDimensions();

  // Make sure mappings are correct.
  ensureMappingsConsistency();
  if (fully_check_mappings)
    fullyCheckMappings();
  if (need_fix_mappings && !fully_check_mappings)
    PLWARNING("In ConcatRowsVMatrix::build_ - Mappings need to be fixed, but you did not set 'fully_check_mappings' to true, this might be dangerous");

  // TODO Note that the 3 lines below will overwrite any provided sizes.
  inputsize_ = to_concat[0]->inputsize();
  targetsize_ = to_concat[0]->targetsize();
  weightsize_ = to_concat[0]->weightsize();
}

/////////
// dot //
/////////
real ConcatRowsVMatrix::dot(int i1, int i2, int inputsize) const
{
  int whichvm1, rowofvm1;
  getpositions(i1,whichvm1,rowofvm1);
  int whichvm2, rowofvm2;
  getpositions(i2,whichvm2,rowofvm2);
  if(whichvm1==whichvm2 && !need_fix_mappings)
    return to_concat[whichvm1]->dot(rowofvm1, rowofvm2, inputsize);
  else
    return VMatrix::dot(i1,i2,inputsize);
}

real ConcatRowsVMatrix::dot(int i, const Vec& v) const
{
  if (!need_fix_mappings) {
    int whichvm, rowofvm;
    getpositions(i,whichvm,rowofvm);
    return to_concat[whichvm]->dot(rowofvm,v);
  }
  else
    return VMatrix::dot(i, v);
}

///////////////////////////////
// ensureMappingsConsistency //
///////////////////////////////
void ConcatRowsVMatrix::ensureMappingsConsistency() {
  // Make sure the string mappings are consistent.
  // For this, we start from the mappings of the first VMat, and add missing
  // mappings obtained from the other VMats. If another VMat has a different
  // mapping for the same string, we remember it in order to fix the output
  // when a getxxxx() method is called.
  need_fix_mappings = false;
  copyStringMappingsFrom(to_concat[0]);
  map<string, real> other_map;
  map<string, real>* cur_map;
  map<string, real>::iterator it, find_map, jt;
  bool report_progress = false; // Turn it on for debugging.
  ProgressBar* pb = 0;
  if (report_progress)
    pb = new ProgressBar("Checking mappings consistency", width());
  for (int j = 0; j < width(); j++) {
    cur_map = &map_sr[j];
    // Find the maximum mapping value for this column.
    real max = -REAL_MAX;
    for (jt = cur_map->begin(); jt != cur_map->end(); jt++)
      if (jt->second > max)
        max = jt->second;
    for (int i = 1; i < to_concat.length(); i++) {
      other_map = to_concat[i]->getStringToRealMapping(j);
      for(it = other_map.begin(); it != other_map.end(); it++) {
        /*
        for (jt = cur_map.begin(); jt != cur_map.end(); jt++) {
          cout << jt->first << " <-> " << jt->second << endl;
        }
        cout << it->first << endl;
        cout << it->second << endl;*/
        find_map = cur_map->find(it->first);
        if (find_map != cur_map->end()) {
          // The string mapped in VMat 'i' is also mapped in our current mapping.
          if (find_map->second != it->second) {
            // But the same string is not mapped to the same value.
            // This needs to be fixed.
            /*cout << find_map->first << endl;
            cout << find_map->second << endl;*/
            need_fix_mappings = true;
            fixed_mappings.resize(to_concat.length(), width());
            fixed_mappings(i, j)[it->second] = find_map->second;
          }
        } else {
          // The string mapped in VMat 'i' is not mapped in our current mapping.
          // We need to add this mapping. But we must make sure there is no
          // existing mapping to the same real number.
          real new_map_val = it->second;
          if (getValString(j, it->second) != "") {
            // There is already a mapping to this real number.
            need_fix_mappings = true;
            fixed_mappings.resize(to_concat.length(), width());
            // We pick for the number the maximum of the current mapped numbers, +1.
            max++;
            /*cout << it->first << endl;
            cout << it->second << endl;
            cout << max << endl;*/
            fixed_mappings(i, j)[it->second] = max;
            new_map_val = max;
          } else {
            // There is no mapping to this real number, it is thus ok to add it.
            if (new_map_val > max)
              max = new_map_val;
            /*cout << it->first << endl;
            cout << it->second << endl;*/
          }
          addStringMapping(j, it->first, new_map_val);
        }
      }
    }
    if (report_progress)
      pb->update(j + 1);
  }
  if (pb)
    delete pb;
}

//////////////////////
// findCommonFields //
//////////////////////
void ConcatRowsVMatrix::findCommonFields() {
  // Find out which fields need to be kept.
  TVec<VMField> final_fields(fieldinfos.length());
  final_fields << fieldinfos;
  TVec<VMField> other_fields;
  TVec<VMField> tmp(final_fields.length());
  for (int i = 1; i < array.length(); i++) {
    map<string, bool> can_be_kept;
    other_fields = array[i]->getFieldInfos();
    for (int j = 0; j < other_fields.length(); j++) {
      can_be_kept[other_fields[j].name] = true;
    }
    tmp.resize(0);
    for (int j = 0; j < final_fields.length(); j++)
      if (can_be_kept.count(final_fields[j].name) > 0)
        tmp.append(final_fields[j]);
    final_fields.resize(tmp.length());
    final_fields << tmp;
  }
  fieldinfos.resize(final_fields.length());
  fieldinfos << final_fields;
  // Get the corresponding field names.
  TVec<string> final_fieldnames(final_fields.length());
  for (int i = 0; i < final_fields.length(); i++)
    final_fieldnames[i] = final_fields[i].name;
  // Now fill 'to_concat' with the selected columns of each VMat in 'array'.
  to_concat.resize(array.length());
  for (int i = 0; i < array.length(); i++)
    to_concat[i] = new SelectColumnsVMatrix(array[i], final_fieldnames);
}

////////////////////////
// fullyCheckMappings //
////////////////////////
void ConcatRowsVMatrix::fullyCheckMappings(bool report_progress) {
  Vec row(width());
  TVec<int> max(width());
  ProgressBar* pb = 0;
  if (report_progress)
    pb = new ProgressBar("Full check of string mappings", length());
  int count = 0;
  for (int i = 0; i < to_concat.length(); i++) {
    for (int j = 0; j < to_concat[i]->length(); j++) {
      to_concat[i]->getRow(j, row);
      for (int k = 0; k < width(); k++) {
        if (!is_missing(row[k]) && map_sr[k].size() > 0) {
          // There is a string mapping for this column.
          // Note that if the value is missing, we should not look for a mapping
          // from this value, because it would find it even if it does not exist
          // (see the STL map documentation to understand why this happens).
          if (to_concat[i]->getValString(k, row[k]) == "") {
            // But it is a numerical value for the i-th VMat.
            if (map_rs[k].find(row[k]) != map_rs[k].end()) {
              // And, unfortunately, we have used this numerical value in
              // the column string mapping. It could be fixed, but
              // this would be pretty annoying, thus we just raise an error.
              PLERROR("In ConcatRowsVMatrix::fullyCheckMappings - In column %s of concatenated VMat number %d, the row %d contains a numerical value (%f) that is used in a string mapping (mapped to %s)", getFieldInfos(k).name.c_str(), i, j, row[k], map_rs[k][row[k]].c_str());
            }
          }
        }
      }
      if (report_progress)
        pb->update(++count);
    }
  }
  if (pb)
    delete pb;
}

/////////
// get //
/////////
real ConcatRowsVMatrix::get(int i, int j) const
{
  static real val;
  int whichvm, rowofvm;
  getpositions(i,whichvm,rowofvm);
  if (!need_fix_mappings || fixed_mappings(whichvm, j).size() == 0)
    return to_concat[whichvm]->get(rowofvm,j);
  else {
    val = to_concat[whichvm]->get(rowofvm,j);
    if (!is_missing(val)) {
      map<real, real>::iterator it = fixed_mappings(whichvm, j).find(val);
      if (it != fixed_mappings(whichvm, j).end()) {
        // We need to modify this value.
        return it->second;
      }
    }
    return val;
  }
}

//////////////////
// getpositions //
//////////////////
void ConcatRowsVMatrix::getpositions(int i, int& whichvm, int& rowofvm) const
{
#ifdef BOUNDCHECK
  if(i<0 || i>=length())
    PLERROR("In ConcatRowsVMatrix::getpositions OUT OF BOUNDS");
#endif

  int pos = 0;
  int k=0;
  while(i>=pos+to_concat[k]->length())
    {
      pos += to_concat[k]->length();
      k++;
    }

  whichvm = k;
  rowofvm = i-pos;
}

///////////////
// getSubRow //
///////////////
void ConcatRowsVMatrix::getSubRow(int i, int j, Vec v) const
{
  static map<real, real> fixed;
  static map<real, real>::iterator it;
  int whichvm, rowofvm;
  getpositions(i,whichvm,rowofvm);
  to_concat[whichvm]->getSubRow(rowofvm, j, v);
  if (need_fix_mappings) {
    for (int k = j; k < j + v.length(); k++) {
      fixed = fixed_mappings(whichvm, k);
      if (!is_missing(v[k-j])) {
        it = fixed.find(v[k -j]);
        if (it != fixed.end())
          v[k - j] = it->second;
      }
    }
  }
}

/////////////////////////////////
// makeDeepCopyFromShallowCopy //
/////////////////////////////////
void ConcatRowsVMatrix::makeDeepCopyFromShallowCopy(CopiesMap& copies) {
  inherited::makeDeepCopyFromShallowCopy(copies);

  // ### Call deepCopyField on all "pointer-like" fields
  // ### that you wish to be deepCopied rather than
  // ### shallow-copied.
  // ### ex:
  // deepCopyField(trainvec, copies);

  // ### Remove this line when you have fully implemented this method.
  PLERROR("ConcatRowsVMatrix::makeDeepCopyFromShallowCopy not fully (correctly) implemented yet!");

}

////////////
// putMat //
////////////
void ConcatRowsVMatrix::putMat(int i, int j, Mat m) {
  int whichvm, rowofvm;
  for (int row = 0; row < length(); row++) {
    getpositions(row + i, whichvm, rowofvm);
    to_concat[whichvm]->putSubRow(rowofvm, j, m(row));
  }
}

////////////////////////
// recomputDimensions //
////////////////////////
void ConcatRowsVMatrix::recomputeDimensions() {
  width_ = to_concat[0]->width();
  length_ = 0;
  for (int i=0; i<to_concat.length(); i++) {
    if (to_concat[i]->width() != width_)
      PLERROR("ConcatRowsVMatrix: underlying-VMat %d has %d width, while 0-th has %d",i,array[i]->width(),width_);
    length_ += to_concat[i]->length();
  }
}

//////////////////////
// reset_dimensions //
//////////////////////
void ConcatRowsVMatrix::reset_dimensions() {
  for (int i=0;i<to_concat.size();i++)
    to_concat[i]->reset_dimensions(); 
  recomputeDimensions();
}


} // end of namespcae PLearn
