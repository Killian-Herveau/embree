// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#pragma once

#include "default.h"
#include "geometry.h"
#include "buffer.h"

namespace embree
{
  /*! represents an array of line segments */
  struct LineSegments : public Geometry
  {
    /*! type of this geometry */
    static const Geometry::Type geom_type = Geometry::LINE_SEGMENTS;

  public:

    /*! line segments construction */
    LineSegments (Device* device);

  public:
    void enabling();
    void disabling();
    void setMask (unsigned mask);
    void setSubtype(RTCGeometrySubtype type);
    void* newBuffer(RTCBufferType type, size_t stride, unsigned int size);
    void setBuffer(RTCBufferType type, void* ptr, size_t offset, size_t stride, unsigned int size);
    void* getBuffer(RTCBufferType type);
    bool verify ();
    void interpolate(const RTCInterpolateArguments* const args);

  public:

    /*! returns the number of vertices */
    __forceinline size_t numVertices() const {
      return vertices[0].size();
    }

    /*! returns the i'th segment */
    __forceinline const unsigned int& segment(size_t i) const {
      return segments[i];
    }

    /*! returns the i'th segment */
    __forceinline unsigned int getStartEndBitMask(size_t i) const {
      unsigned int mask = 0;
      if (flags) 
        mask |= (flags[i] & 0x3) << 30;
      return mask;
    }

     /*! returns i'th vertex of the first time step */
    __forceinline Vec3fa vertex(size_t i) const {
      return vertices0[i];
    }

    /*! returns i'th vertex of the first time step */
    __forceinline const char* vertexPtr(size_t i) const {
      return vertices0.getPtr(i);
    }

    /*! returns i'th radius of the first time step */
    __forceinline float radius(size_t i) const {
      return vertices0[i].w;
    }

    /*! returns i'th vertex of itime'th timestep */
    __forceinline Vec3fa vertex(size_t i, size_t itime) const {
      return vertices[itime][i];
    }

    /*! returns i'th vertex of itime'th timestep */
    __forceinline const char* vertexPtr(size_t i, size_t itime) const {
      return vertices[itime].getPtr(i);
    }

    /*! returns i'th radius of itime'th timestep */
    __forceinline float radius(size_t i, size_t itime) const {
      return vertices[itime][i].w;
    }

    /*! calculates bounding box of i'th line segment */
    __forceinline BBox3fa bounds(size_t i) const
    {
      const unsigned int index = segment(i);
      const float r0 = radius(index+0);
      const float r1 = radius(index+1);
      const Vec3fa v0 = vertex(index+0);
      const Vec3fa v1 = vertex(index+1);
      const BBox3fa b = merge(BBox3fa(v0),BBox3fa(v1));
      return enlarge(b,Vec3fa(max(r0,r1)));
    }

    /*! calculates bounding box of i'th line segment for the itime'th time step */
    __forceinline BBox3fa bounds(size_t i, size_t itime) const
    {
      const unsigned int index = segment(i);
      const float r0 = radius(index+0,itime);
      const float r1 = radius(index+1,itime);
      const Vec3fa v0 = vertex(index+0,itime);
      const Vec3fa v1 = vertex(index+1,itime);
      const BBox3fa b = merge(BBox3fa(v0),BBox3fa(v1));
      return enlarge(b,Vec3fa(max(r0,r1)));
    }

    /*! check if the i'th primitive is valid at the itime'th timestep */
    __forceinline bool valid(size_t i, size_t itime) const {
      return valid(i, make_range(itime, itime));
    }

    /*! check if the i'th primitive is valid between the specified time range */
    __forceinline bool valid(size_t i, const range<size_t>& itime_range) const
    {
      const unsigned int index = segment(i);
      if (index+1 >= numVertices()) return false;
      
      for (size_t itime = itime_range.begin(); itime <= itime_range.end(); itime++)
      {
        const Vec3fa v0 = vertex(index+0,itime); if (unlikely(!isvalid((vfloat4)v0))) return false;
        const Vec3fa v1 = vertex(index+1,itime); if (unlikely(!isvalid((vfloat4)v1))) return false;
        if (min(v0.w,v1.w) < 0.0f) return false;
      }
      return true;
    }

    /*! calculates the linear bounds of the i'th primitive at the itimeGlobal'th time segment */
    __forceinline LBBox3fa linearBounds(size_t i, size_t itime) const {
      return LBBox3fa(bounds(i,itime+0),bounds(i,itime+1));
    }

    /*! calculates the build bounds of the i'th primitive, if it's valid */
    __forceinline bool buildBounds(size_t i, BBox3fa* bbox) const
    {
      if (!valid(i,0)) return false;
      *bbox = bounds(i); 
      return true;
    }

    /*! calculates the build bounds of the i'th primitive at the itime'th time segment, if it's valid */
    __forceinline bool buildBounds(size_t i, size_t itime, BBox3fa& bbox) const
    {
      if (!valid(i,itime+0) || !valid(i,itime+1)) return false;
      bbox = bounds(i,itime);  // use bounds of first time step in builder
      return true;
    }

    /*! calculates the linear bounds of the i'th primitive for the specified time range */
    __forceinline LBBox3fa linearBounds(size_t primID, const BBox1f& time_range) const {
      return LBBox3fa([&] (size_t itime) { return bounds(primID, itime); }, time_range, fnumTimeSegments);
    }

    /*! calculates the linear bounds of the i'th primitive for the specified time range */
    __forceinline bool linearBounds(size_t i, const BBox1f& time_range, LBBox3fa& bbox) const
    {
      if (!valid(i, getTimeSegmentRange(time_range, fnumTimeSegments))) return false;
      bbox = linearBounds(i, time_range);
      return true;
    }

  public:
    Buffer<unsigned int> segments;                 //!< array of line segment indices
    BufferView<Vec3fa> vertices0;                  //!< fast access to first vertex buffer
    Buffer<char> flags;                            //!< start, end flag per segment
    vector<Buffer<Vec3fa>> vertices;               //!< vertex array for each timestep
    vector<Buffer<char>> userbuffers;              //!< user buffers
  };

  namespace isa
  {
    struct LineSegmentsISA : public LineSegments
    {
      LineSegmentsISA (Device* device)
        : LineSegments(device) {}
    };
  }

  DECLARE_ISA_FUNCTION(LineSegments*, createLineSegments, Device*);
}
