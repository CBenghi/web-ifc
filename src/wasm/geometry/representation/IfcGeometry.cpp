/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at https://mozilla.org/MPL/2.0/.  */

// Implementation for IfcGeometry

#include "IfcGeometry.h"

namespace webifc::geometry {


	void IfcGeometry::ReverseFace(uint32_t index)
	{
			fuzzybools::Face f = GetFace(index);
			indexData[index * 3 + 0] = f.i2;
			indexData[index * 3 + 1] = f.i1;
			indexData[index * 3 + 2] = f.i0;
	}

	void IfcGeometry::ReverseFaces()
	{
		for (size_t i = 0; i < numFaces; i++)
		{
			ReverseFace(i);
		}
	}

	uint32_t IfcGeometry::GetVertexData()
	{
		// unfortunately webgl can't do doubles
		if (fvertexData.size() != vertexData.size())
		{
			fvertexData.resize(vertexData.size());
			// vertexData is a multiple of VERTEX_FORMAT_SIZE_FLOATS, which can change
			// from version to version, we trust Fuzzy-Bool and copy the whole collection
			for (size_t i = 0; i < vertexData.size(); i ++)
			{
				fvertexData[i] = vertexData[i];
			}

			// cleanup
			// vertexData = {};
		}

		if (fvertexData.empty())
		{
			return 0;
		}

		return (uint32_t)(size_t)&fvertexData[0];
	}

	void IfcGeometry::AddGeometry(fuzzybools::Geometry geom)
	{
			
		for (uint32_t i = 0; i < geom.numFaces; i++)
		{
			fuzzybools::Face f = geom.GetFace(i);
			glm::dvec3 a = geom.GetPoint(f.i0);
			glm::dvec3 b = geom.GetPoint(f.i1);
			glm::dvec3 c = geom.GetPoint(f.i2);
			AddFace(a, b, c);
		}

	}

	uint32_t IfcGeometry::GetVertexDataSize()
	{
		return (uint32_t)fvertexData.size();
	}

	uint32_t IfcGeometry::GetIndexData()
	{
		return (uint32_t)(size_t)&indexData[0];
	}

	uint32_t IfcGeometry::GetIndexDataSize()
	{
		return (uint32_t)indexData.size();
	}

}