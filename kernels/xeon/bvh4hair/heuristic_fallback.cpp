// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "heuristic_fallback.h"

namespace embree
{
  FallBackSplit FallBackSplit::find(size_t threadIndex, PrimRefBlockAlloc<Bezier1>& alloc, BezierRefList& prims, 
				    BezierRefList& lprims_o, BezierRefList& rprims_o)
  {
    size_t num = 0, lnum = 0, rnum = 0;
    BBox3fa lbounds = empty, rbounds = empty;
    BezierRefList::item* lblock = lprims_o.insert(alloc.malloc(threadIndex));
    BezierRefList::item* rblock = rprims_o.insert(alloc.malloc(threadIndex));
    
    while (BezierRefList::item* block = prims.take()) 
    {
      for (size_t i=0; i<block->size(); i++) 
      {
        const Bezier1& prim = block->at(i); 
	const BBox3fa bounds = prim.bounds();
	
        if ((num++)%2) 
        {
          lnum++;
	  lbounds.extend(bounds);
	  
          if (likely(lblock->insert(prim))) continue; 
          lblock = lprims_o.insert(alloc.malloc(threadIndex));
          lblock->insert(prim);
        } 
        else 
        {
          rnum++;
	  rbounds.extend(bounds);
	  
          if (likely(rblock->insert(prim))) continue;
          rblock = rprims_o.insert(alloc.malloc(threadIndex));
          rblock->insert(prim);
        }
      }
      alloc.free(threadIndex,block);
    }
    //const NAABBox3fa bounds0 = computeAlignedBounds(lprims_o);
    //const NAABBox3fa bounds1 = computeAlignedBounds(rprims_o);
    return FallBackSplit(lbounds,lnum,rbounds,rnum);
  }
}
