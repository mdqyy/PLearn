// -*- C++ -*-

// PLearn (A C++ Machine Learning Library)
// Copyright (C) 1998 Pascal Vincent
// Copyright (C) 1999-2002 Pascal Vincent, Yoshua Bengio and University of Montreal
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
   * $Id: RealMapping.cc,v 1.1 2002/07/30 09:01:26 plearner Exp $
   * This file is part of the PLearn library.
   ******************************************************* */

#include "RealMapping.h"
#include "fileutils.h"

namespace PLearn <%
using namespace std;

  IMPLEMENT_NAME_AND_DEEPCOPY(RealMapping);

  real RealMapping::map(real val) const
  {
    if(is_missing(val))
      return missing_mapsto;
    mapping_t::iterator it = mapping.begin();
    mapping_t::iterator itend = mapping.end();
    for(; it!=itend; ++it)
      if(it->first.contains(val))
        return it->second;
    if(keep_other_as_is)
      return val;
    else
      return other_mapsto;      
  }

  void RealMapping::transform(const Vec& v) const
  {
    real* p = v.data();
    int l = v.length();
    while(l--)
      {
        *p = map(*p);
        ++p;
      }
  }

  void RealMapping::print(ostream& out) const
  { 
    out << "{ ";
    int n = mapping.length();
    out.precision(17);
    for(int i=0; i<n; i++)
      out << mapping[i].first << " -> " << mapping[i].second << "; ";
    if(!is_missing(missing_mapsto))
      out << "MISSING -> " << missing_mapsto << "; " << endl;
    if(!keep_other_as_is)
      out << "OTHER -> " << other_mapsto;
    out << "} ";
  }

  void RealMapping::write(ostream& out) const
  { 
    writeHeader(out,"RealMapping",0);
    out << "{ ";
    int n = mapping.length();
    out.precision(17);
    for(int i=0; i<n; i++)
      out << mapping[i].first << " -> " << mapping[i].second << "; ";
    if(!is_missing(missing_mapsto))
      out << "MISSING -> " << missing_mapsto << "; " << endl;
    if(!keep_other_as_is)
      out << "OTHER -> " << other_mapsto;
    out << "} ";
    writeFooter(out,"RealMapping");
  }

  void RealMapping::read(istream& in)
  {
    mapping.resize(0);
    missing_mapsto = MISSING_VALUE;
    keep_other_as_is = true;

    skipBlanks(in);
    char c=in.get();
    bool uses_headers = false;
    
    if(c=='<')
      {
        in.unget();
        int version = readHeader(in,"RealMapping");
        if(version!=0)
          PLERROR("In RealMapping::read reading of version #%d not implemented",version);
        uses_headers = true;
        skipBlanks(in);
        c = in.get();
      }

    if(c!='{')
      PLERROR("In RealMapping::read, expected a {, found a %c",c);

    while((c=peekAfterSkipBlanks(in))!='}' && !in.eof())
      {
        RealRange r;
            
        if(c==';'){c=in.get();}
        else if(c=='[' || c==']')
          {
            r.read(in);
            skipBlanks(in);
            if(in.get()!='-' || in.get()!='>')
                PLERROR("Expecting -> after range specification ( range syntax example : ]100 200] -> 10 )");
            real val;
            in >> val;
            addMapping(r,val);
          }
	else if (isdigit(c) || c == '-' || c == '.')
	  {
	    real val0, val;
	    in >> val0 >> ws;
	    r= RealRange('[', val0, val0, ']');
            if(in.get()!='-' || in.get()!='>')
                PLERROR("Expecting -> after range specification ( range syntax example : ]100 200] -> 10 )");
            in >> val;
            addMapping(r,val);	    
	  }
       else 
          {
            string str;
            getline(in,str,'-');
            str=removeblanks(str);
            if(str=="MISSING")
              {
                if(in.get()!='>')
                  PLERROR("Expecting -> after MISSING");
                real val;
                in >> val;
                setMappingForMissing(val);
              }
            else if (str=="OTHER")
              {
                if(in.get()!='>')
                  PLERROR("Expecting -> after OTHER");
                real val;
                in >> val;
                setMappingForOther(val);
              }
            else PLERROR("Unexpected string in mapping definition : %s",str.c_str());
          }
      }
    in.get();

    if(uses_headers)
      {
        skipBlanks(in);
        readFooter(in,"RealMapping");        
      }
  }

%> // end of namespace PLearn
