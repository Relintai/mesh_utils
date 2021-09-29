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
#include "core/local_vector.h"
#include "core/variant.h"
#include "scene/resources/mesh.h"

#include visual_server_h

#include "xatlas/xatlas.h"

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
				bool bequals = true;

				int indx = i * 4;

				for (int l = 0; l < 4; ++l) {
					if (bones[indx + l] != bone[l]) {
						bequals = false;
						break;
					}

					if (!Math::is_equal_approx(weights[indx + l], weight[l])) {
						bequals = false;
						break;
					}
				}

				if (!bequals) {
					continue;
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

	Array retarr;
	retarr.resize(VisualServer::ARRAY_MAX);

	retarr[VisualServer::ARRAY_VERTEX] = verts;

	if (normals.size() > 0)
		retarr[VisualServer::ARRAY_NORMAL] = normals;
	if (uvs.size() > 0)
		retarr[VisualServer::ARRAY_TEX_UV] = uvs;
	if (colors.size() > 0)
		retarr[VisualServer::ARRAY_COLOR] = colors;
	if (indices.size() > 0)
		retarr[VisualServer::ARRAY_INDEX] = indices;
	if (bones.size() > 0)
		retarr[VisualServer::ARRAY_BONES] = bones;
	if (weights.size() > 0)
		retarr[VisualServer::ARRAY_WEIGHTS] = weights;

	return retarr;
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
				bool bequals = true;

				int indx = i * 4;

				for (int l = 0; l < 4; ++l) {
					if (bones[indx + l] != bone[l]) {
						bequals = false;
						break;
					}

					if (!Math::is_equal_approx(weights[indx + l], weight[l])) {
						bequals = false;
						break;
					}
				}

				if (!bequals) {
					continue;
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

	Array retarr;
	retarr.resize(VisualServer::ARRAY_MAX);

	retarr[VisualServer::ARRAY_VERTEX] = verts;

	if (normals.size() > 0)
		retarr[VisualServer::ARRAY_NORMAL] = normals;
	if (uvs.size() > 0)
		retarr[VisualServer::ARRAY_TEX_UV] = uvs;
	if (colors.size() > 0)
		retarr[VisualServer::ARRAY_COLOR] = colors;
	if (indices.size() > 0)
		retarr[VisualServer::ARRAY_INDEX] = indices;
	if (bones.size() > 0)
		retarr[VisualServer::ARRAY_BONES] = bones;
	if (weights.size() > 0)
		retarr[VisualServer::ARRAY_WEIGHTS] = weights;

	return retarr;
}

PoolVector2Array MeshUtils::uv_unwrap(Array arrays, bool p_block_align, float p_texel_size, int p_padding, int p_max_chart_size) const {
	LocalVector<float> vertices;
	LocalVector<float> normals;
	LocalVector<int> indices;

	PoolVector<Vector3> rvertices = arrays[Mesh::ARRAY_VERTEX];
	int vc = rvertices.size();
	PoolVector<Vector3>::Read r = rvertices.read();

	PoolVector<Vector3> rnormals = arrays[Mesh::ARRAY_NORMAL];
	PoolVector<Vector3>::Read rn = rnormals.read();

	int vertex_ofs = vertices.size() / 3;

	vertices.resize((vertex_ofs + vc) * 3);
	normals.resize((vertex_ofs + vc) * 3);

	for (int j = 0; j < vc; j++) {
		Vector3 v = r[j];
		Vector3 n = rn[j];

		vertices[(j + vertex_ofs) * 3 + 0] = v.x;
		vertices[(j + vertex_ofs) * 3 + 1] = v.y;
		vertices[(j + vertex_ofs) * 3 + 2] = v.z;
		normals[(j + vertex_ofs) * 3 + 0] = n.x;
		normals[(j + vertex_ofs) * 3 + 1] = n.y;
		normals[(j + vertex_ofs) * 3 + 2] = n.z;
	}

	PoolVector<int> rindices = arrays[Mesh::ARRAY_INDEX];
	int ic = rindices.size();

	float eps = 1.19209290e-7F; // Taken from xatlas.h
	if (ic == 0) {
		for (int j = 0; j < vc / 3; j++) {
			Vector3 p0 = r[j * 3 + 0];
			Vector3 p1 = r[j * 3 + 1];
			Vector3 p2 = r[j * 3 + 2];

			if ((p0 - p1).length_squared() < eps || (p1 - p2).length_squared() < eps || (p2 - p0).length_squared() < eps) {
				continue;
			}

			indices.push_back(vertex_ofs + j * 3 + 0);
			indices.push_back(vertex_ofs + j * 3 + 1);
			indices.push_back(vertex_ofs + j * 3 + 2);
		}

	} else {
		PoolVector<int>::Read ri = rindices.read();

		for (int j = 0; j < ic / 3; j++) {
			Vector3 p0 = r[ri[j * 3 + 0]];
			Vector3 p1 = r[ri[j * 3 + 1]];
			Vector3 p2 = r[ri[j * 3 + 2]];

			if ((p0 - p1).length_squared() < eps || (p1 - p2).length_squared() < eps || (p2 - p0).length_squared() < eps) {
				continue;
			}

			indices.push_back(vertex_ofs + ri[j * 3 + 0]);
			indices.push_back(vertex_ofs + ri[j * 3 + 1]);
			indices.push_back(vertex_ofs + ri[j * 3 + 2]);
		}
	}

	// set up input mesh
	xatlas_mu::MeshDecl input_mesh;
	input_mesh.indexData = indices.ptr();
	input_mesh.indexCount = indices.size();
	input_mesh.indexFormat = xatlas_mu::IndexFormat::UInt32;

	input_mesh.vertexCount = vertices.size() / 3;
	input_mesh.vertexPositionData = vertices.ptr();
	input_mesh.vertexPositionStride = sizeof(float) * 3;
	input_mesh.vertexNormalData = normals.ptr();
	input_mesh.vertexNormalStride = sizeof(uint32_t) * 3;
	input_mesh.vertexUvData = nullptr;
	input_mesh.vertexUvStride = 0;

	xatlas_mu::ChartOptions chart_options;
	//not sure whether this is better off as true or false, since I don't copy back the indices
	//I'm leaving it on off for now
	//TODO if the generated uvs have weird problems try to set this to true
	chart_options.fixWinding = false;

	xatlas_mu::PackOptions pack_options;
	pack_options.padding = p_padding;
	pack_options.maxChartSize = p_max_chart_size; // Lightmap atlassing needs 2 for padding between meshes, so 4096-2
	pack_options.blockAlign = p_block_align;
	pack_options.texelsPerUnit = 1.0 / p_texel_size;

	xatlas_mu::Atlas *atlas = xatlas_mu::Create();

	xatlas_mu::AddMeshError err = xatlas_mu::AddMesh(atlas, input_mesh, 1);
	ERR_FAIL_COND_V_MSG(err != xatlas_mu::AddMeshError::Success, PoolVector2Array(), xatlas_mu::StringForEnum(err));

	xatlas_mu::Generate(atlas, chart_options, pack_options);

	float w = atlas->width;
	float h = atlas->height;

	if (w == 0 || h == 0) {
		xatlas_mu::Destroy(atlas);
		return PoolVector2Array(); //could not bake because there is no area
	}

	const xatlas_mu::Mesh &output = atlas->meshes[0];

	PoolVector2Array retarr;
	retarr.resize(output.vertexCount);

	PoolVector2Array::Write retarrw = retarr.write();

	for (uint32_t i = 0; i < output.vertexCount; i++) {
		int vind = output.vertexArray[i].xref;

		retarrw[vind] = Vector2(output.vertexArray[i].uv[0] / w, output.vertexArray[i].uv[1] / h);
	}

	retarrw.release();

	xatlas_mu::Destroy(atlas);

	return retarr;
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

	ClassDB::bind_method(D_METHOD("uv_unwrap", "arr", "block_align", "texel_size", "padding", "max_chart_size"), &MeshUtils::uv_unwrap, true, 0.05, 1, 4094);
}

#if GODOT4
#undef Texture
#endif