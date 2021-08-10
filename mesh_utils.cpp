/*
Copyright (c) 2019-2021 Péter Magyar

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

#include visual_server_h

#if GODOT4
#define Texture Texture2D
#endif

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

	if (normals.size() > 0)
		arr[VisualServer::ARRAY_NORMAL] = normals;
	if (uvs.size() > 0)
		arr[VisualServer::ARRAY_TEX_UV] = uvs;
	if (colors.size() > 0)
		arr[VisualServer::ARRAY_COLOR] = colors;
	if (indices.size() > 0)
		arr[VisualServer::ARRAY_INDEX] = indices;
	if (bones.size() > 0)
		arr[VisualServer::ARRAY_BONES] = bones;
	if (weights.size() > 0)
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

//If normals are present they need to match too to be removed
Array MeshUtils::remove_doubles(Array arr) const {
	ERR_FAIL_COND_V(arr.size() != VisualServer::ARRAY_MAX, arr);

	PoolVector3Array verts = arr[VisualServer::ARRAY_VERTEX];
	PoolVector3Array normals = arr[VisualServer::ARRAY_NORMAL];
	PoolVector2Array uvs = arr[VisualServer::ARRAY_TEX_UV];
	PoolColorArray colors = arr[VisualServer::ARRAY_COLOR];
	PoolIntArray indices = arr[VisualServer::ARRAY_INDEX];
	PoolIntArray bones = arr[VisualServer::ARRAY_BONES];
	PoolRealArray weights = arr[VisualServer::ARRAY_WEIGHTS];

	ERR_FAIL_COND_V(normals.size() != 0 && normals.size() != verts.size(), Array());
	ERR_FAIL_COND_V(uvs.size() != 0 && normals.size() != verts.size(), Array());
	ERR_FAIL_COND_V(colors.size() != 0 && normals.size() != verts.size(), Array());
	ERR_FAIL_COND_V(bones.size() != 0 && bones.size() != (verts.size() * 4), Array());
	ERR_FAIL_COND_V(weights.size() != 0 && weights.size() != (verts.size() * 4), Array());
	ERR_FAIL_COND_V(bones.size() != weights.size(), Array());

	Vector3 v;
	Vector3 normal;
	Vector2 uv;
	Color color;
	PoolIntArray bone;
	bone.resize(4);
	PoolRealArray weight;
	weight.resize(4);

	int i = 0;
	while (i < verts.size()) {
		v = verts[i];

		if (normals.size() != 0) {
			normal = normals[i];
		}

		if (uvs.size() != 0) {
			uv = uvs[i];
		}

		if (colors.size() != 0) {
			color = colors[i];
		}

		if (bones.size() != 0) {
			int indx = i * 4;

			for (int l = 0; l < 4; ++l) {
				bone.set(l, bones[indx + l]);
				weight.set(l, weights[indx + l]);
			}
		}

		Array equals;
		for (int j = i + 1; j < verts.size(); ++j) {
			Vector3 vc = verts[j];

			if (normals.size() != 0) {
				if (!normals[j].is_equal_approx(normal)) {
					continue;
				}
			}

			if (uvs.size() != 0) {
				if (!uvs[j].is_equal_approx(uv)) {
					continue;
				}
			}

			if (colors.size() != 0) {
				if (!colors[j].is_equal_approx(color)) {
					continue;
				}
			}

			if (bones.size() != 0) {
				int indx = i * 4;

				for (int l = 0; l < 4; ++l) {
					if (bones[indx + l] != bone[l]) {
						continue;
					}

					if (!Math::is_equal_approx(weights[indx + l], weight[l])) {
						continue;
					}
				}
			}

			if (vc.is_equal_approx(v)) {
				equals.push_back(j);
			}
		}

		for (int k = 0; k < equals.size(); ++k) {
			int rem = equals[k];
			int remk = rem - k;

			verts.remove(remk);

			if (normals.size() > 0) {
				normals.remove(remk);
			}

			if (uvs.size() > 0) {
				uvs.remove(remk);
			}

			if (colors.size() > 0) {
				colors.remove(remk);
			}

			if (bones.size() > 0) {
				int bindex = remk * 4;
				for (int l = 0; l < 4; ++l) {
					bones.remove(bindex);
					weights.remove(bindex);
				}
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

	if (normals.size() > 0)
		arr[VisualServer::ARRAY_NORMAL] = normals;
	if (uvs.size() > 0)
		arr[VisualServer::ARRAY_TEX_UV] = uvs;
	if (colors.size() > 0)
		arr[VisualServer::ARRAY_COLOR] = colors;
	if (indices.size() > 0)
		arr[VisualServer::ARRAY_INDEX] = indices;
	if (bones.size() > 0)
		arr[VisualServer::ARRAY_BONES] = bones;
	if (weights.size() > 0)
		arr[VisualServer::ARRAY_WEIGHTS] = weights;

	return arr;
}

//Normals are always interpolated, merged
Array MeshUtils::remove_doubles_interpolate_normals(Array arr) const {
	ERR_FAIL_COND_V(arr.size() != VisualServer::ARRAY_MAX, arr);

	PoolVector3Array verts = arr[VisualServer::ARRAY_VERTEX];
	PoolVector3Array normals = arr[VisualServer::ARRAY_NORMAL];
	PoolVector2Array uvs = arr[VisualServer::ARRAY_TEX_UV];
	PoolColorArray colors = arr[VisualServer::ARRAY_COLOR];
	PoolIntArray indices = arr[VisualServer::ARRAY_INDEX];
	PoolIntArray bones = arr[VisualServer::ARRAY_BONES];
	PoolRealArray weights = arr[VisualServer::ARRAY_WEIGHTS];

	ERR_FAIL_COND_V(normals.size() != 0 && normals.size() != verts.size(), Array());
	ERR_FAIL_COND_V(uvs.size() != 0 && normals.size() != verts.size(), Array());
	ERR_FAIL_COND_V(colors.size() != 0 && normals.size() != verts.size(), Array());
	ERR_FAIL_COND_V(bones.size() != 0 && bones.size() != (verts.size() * 4), Array());
	ERR_FAIL_COND_V(weights.size() != 0 && weights.size() != (verts.size() * 4), Array());
	ERR_FAIL_COND_V(bones.size() != weights.size(), Array());

	Vector3 v;
	Vector2 uv;
	Color color;
	PoolIntArray bone;
	bone.resize(4);
	PoolRealArray weight;
	weight.resize(4);

	int i = 0;
	while (i < verts.size()) {
		v = verts[i];

		if (uvs.size() != 0) {
			uv = uvs[i];
		}

		if (colors.size() != 0) {
			color = colors[i];
		}

		if (bones.size() != 0) {
			int indx = i * 4;

			for (int l = 0; l < 4; ++l) {
				bone.set(l, bones[indx + l]);
				weight.set(l, weights[indx + l]);
			}
		}

		Array equals;
		for (int j = i + 1; j < verts.size(); ++j) {
			Vector3 vc = verts[j];

			if (uvs.size() != 0) {
				if (!uvs[j].is_equal_approx(uv)) {
					continue;
				}
			}

			if (colors.size() != 0) {
				if (!colors[j].is_equal_approx(color)) {
					continue;
				}
			}

			if (bones.size() != 0) {
				int indx = i * 4;

				for (int l = 0; l < 4; ++l) {
					if (bones[indx + l] != bone[l]) {
						continue;
					}

					if (!Math::is_equal_approx(weights[indx + l], weight[l])) {
						continue;
					}
				}
			}

			if (vc.is_equal_approx(v)) {
				equals.push_back(j);
			}
		}

		Vector3 normal;
		for (int k = 0; k < equals.size(); ++k) {
			int rem = equals[k];
			int remk = rem - k;

			verts.remove(remk);

			if (normals.size() > 0) {
				Vector3 n = normals[remk];
				normals.remove(remk);

				if (k == 0) {
					normal = n;
				} else {
					normal = normal.linear_interpolate(n, 0.5);
				}
			}

			if (uvs.size() > 0) {
				uvs.remove(remk);
			}

			if (colors.size() > 0) {
				colors.remove(remk);
			}

			if (bones.size() > 0) {
				int bindex = remk * 4;
				for (int l = 0; l < 4; ++l) {
					bones.remove(bindex);
					weights.remove(bindex);
				}
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

	if (normals.size() > 0)
		arr[VisualServer::ARRAY_NORMAL] = normals;
	if (uvs.size() > 0)
		arr[VisualServer::ARRAY_TEX_UV] = uvs;
	if (colors.size() > 0)
		arr[VisualServer::ARRAY_COLOR] = colors;
	if (indices.size() > 0)
		arr[VisualServer::ARRAY_INDEX] = indices;
	if (bones.size() > 0)
		arr[VisualServer::ARRAY_BONES] = bones;
	if (weights.size() > 0)
		arr[VisualServer::ARRAY_WEIGHTS] = weights;

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

	ClassDB::bind_method(D_METHOD("remove_doubles", "arr"), &MeshUtils::remove_doubles);
	ClassDB::bind_method(D_METHOD("remove_doubles_interpolate_normals", "arr"), &MeshUtils::remove_doubles_interpolate_normals);
}

#if GODOT4
#undef Texture
#endif