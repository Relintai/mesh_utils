/*
Copyright (c) 2019-2020 Péter Magyar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRMeshUtils OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNMeshUtils FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "mesh_utils.h"

#include "defines.h"
#include visual_server_h

MeshUtils *MeshUtils::_instance;

MeshUtils *MeshUtils::get_singleton() {
	return _instance;
}

Array MeshUtils::merge_mesh_array(Array arr) const {
	ERR_FAIL_COND_V(arr.size() != VisualServer::ARRAY_MAX, arr);

	PoolVector3Array verts = arr[VisualServer::ARRAY_VERTEX];
	PoolVector3Array normals = arr[VisualServer::ARRAY_NORMAL];
	PoolVector2Array uvs = arr[VisualServer::ARRAY_TEX_UV];
	PoolColorArray colors = arr[VisualServer::ARRAY_COLOR];
	PoolIntArray indices = arr[VisualServer::ARRAY_INDEX];
	PoolIntArray bones = arr[VisualServer::ARRAY_BONES];
	PoolRealArray weights = arr[VisualServer::ARRAY_WEIGHTS];

	int i = 0;
	while (i < verts.size()) {
		Vector3 v = verts[i];

		Array equals;
		for (int j = i + 1; j < verts.size(); ++j) {
			Vector3 vc = verts[j];

			if (Math::is_equal_approx(v.x, vc.x) && Math::is_equal_approx(v.y, vc.y) && Math::is_equal_approx(v.z, vc.z))
				equals.push_back(j);
		}

		for (int k = 0; k < equals.size(); ++k) {
			int rem = equals[k];
			int remk = rem - k;

			verts.remove(remk);
			normals.remove(remk);
			uvs.remove(remk);
			colors.remove(remk);

			int bindex = remk * 4;
			for (int l = 0; l < 4; ++l) {
				bones.remove(bindex);
				weights.remove(bindex);
			}

			for (int j = 0; j < indices.size(); ++j) {
				int indx = indices[j];

				if (indx == remk)
					indices.set(j, i);
				else if (indx > remk)
					indices.set(j, indx - 1);
			}
		}

		++i;
	}

	arr[VisualServer::ARRAY_VERTEX] = verts;
	arr[VisualServer::ARRAY_NORMAL] = normals;
	arr[VisualServer::ARRAY_TEX_UV] = uvs;
	arr[VisualServer::ARRAY_COLOR] = colors;
	arr[VisualServer::ARRAY_INDEX] = indices;
	arr[VisualServer::ARRAY_BONES] = bones;
	arr[VisualServer::ARRAY_WEIGHTS] = weights;

	return arr;
}
Array MeshUtils::bake_mesh_array_uv(Array arr, Ref<Texture> tex, float mul_color) const {
	ERR_FAIL_COND_V(arr.size() != VisualServer::ARRAY_MAX, arr);
	ERR_FAIL_COND_V(!tex.is_valid(), arr);

	Ref<Image> img = tex->get_data();

	ERR_FAIL_COND_V(!img.is_valid(), arr);

	Vector2 imgsize = img->get_size();

	PoolVector2Array uvs = arr[VisualServer::ARRAY_TEX_UV];
	PoolColorArray colors = arr[VisualServer::ARRAY_COLOR];

#if !GODOT4
	img->lock();
#endif

	for (int i = 0; i < uvs.size(); ++i) {
		Vector2 uv = uvs[i];
		uv *= imgsize;

		Color c = img->get_pixelv(uv);

		colors.set(i, colors[i] * c * mul_color);
	}

#if !GODOT4
	img->unlock();
#endif

	arr[VisualServer::ARRAY_COLOR] = colors;

	return arr;
}

MeshUtils::MeshUtils() {
	_instance = this;
}

MeshUtils::~MeshUtils() {
	_instance = NULL;
}

void MeshUtils::_bind_methods() {
	ClassDB::bind_method(D_METHOD("merge_mesh_array", "arr"), &MeshUtils::merge_mesh_array);
	ClassDB::bind_method(D_METHOD("bake_mesh_array_uv", "arr", "tex", "mul_color"), &MeshUtils::bake_mesh_array_uv, DEFVAL(0.7));
}
