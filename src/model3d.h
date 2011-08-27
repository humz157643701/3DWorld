// 3D World - 3D Model Rendering Classes
// by Frank Gennari
// 8/17/11

#ifndef _MODEL3D_H_
#define _MODEL3D_H_

#include "3DWorld.h"

using namespace std;

typedef map<string, unsigned> string_map_t;

colorRGB const def_color(0.0, 0.0, 0.0);


struct geom_xform_t {
	vector3d tv;
	float scale;
	bool mirror[3], swap_dim[3][3];

	geom_xform_t() : tv(zero_vector), scale(1.0) {
		for (unsigned i = 0; i < 3; ++i) {
			UNROLL_3X(swap_dim[i][i_] = 0;)
			mirror[i] = 0;
		}
	}
	void xform_pos_rm(point &pos) const {
		UNROLL_3X(if (mirror[i_]) pos[i_] = -pos[i_];)
		
		for (unsigned i = 0; i < 3; ++i) {
			UNROLL_3X(if (swap_dim[i][i_]) swap(pos[i], pos[i_]);)
		}
	}
	void xform_pos_rms(point &pos) const {
		xform_pos_rm(pos);
		pos *= scale;
	}
	void xform_pos(point &pos) const {
		xform_pos_rms(pos);
		pos += tv;
	}
	void xform_vect(vector<point> &v) const {
		for (vector<point>::iterator i = v.begin(); i != v.end(); ++i) {
			xform_pos(*i);
		}
	}
};


class vntc_vect_t : public vector<vert_norm_tc> {

	unsigned vbo;

public:
	vntc_vect_t() : vbo(0) {}
	void render(bool is_shadow_pass) const;
	void render_array(bool is_shadow_pass, int prim_type);
	void free_vbo();
	bool is_convex() const;
	vector3d get_planar_normal() const;
	bool is_valid() const {return (size() >= 3 && is_triangle_valid((*this)[0].v, (*this)[1].v, (*this)[2].v));}
	void from_points(vector<point> const &pts);
	void remove_excess_cap() {if (size() < capacity()) vector<vert_norm_tc>(*this).swap(*this);}
};


class polygon_t : public vntc_vect_t {

public:
	colorRGBA color;

	polygon_t(colorRGBA const &c=ALPHA0) : color(c) {}
	polygon_t(vntc_vect_t const &vv, colorRGBA const &c=ALPHA0) : vntc_vect_t(vv), color(c) {}
};


struct geometry_t {

	vntc_vect_t triangles, quads;

	void render(bool is_shadow_pass);
	bool empty() const {return (triangles.empty() && quads.empty());}
	void add_poly(vntc_vect_t const &poly);
	void remove_excess_cap();
	void free_vbos();
	void clear();
};


class texture_manager {

protected:
	deque<texture_t> textures;
	string_map_t tex_map; // maps texture filenames to texture indexes

public:
	unsigned create_texture(string const &fn, bool is_alpha_mask, bool verbose);
	void clear();
	void free_tids();
	void free_textures();
	void ensure_texture_loaded(texture_t &t);
	void bind_alpha_channel_to_texture(int tid, int alpha_tid);
	void ensure_tid_loaded(int tid);
	void ensure_tid_bound(int tid);
	void bind_texture(int tid) const;
	colorRGBA get_tex_avg_color(int tid) const;
};


struct material_t {

	colorRGB ka, kd, ks, ke, tf;
	float ns, ni, alpha, tr;
	unsigned illum;
	int a_tid, d_tid, s_tid, alpha_tid, bump_tid;
	bool skip, is_used;

	// geometry - does this go here or somewhere else?
	geometry_t geom;

	material_t() : ka(def_color), kd(def_color), ks(def_color), ke(def_color), tf(def_color), ns(1.0), ni(1.0), alpha(1.0), tr(0.0),
		illum(2), a_tid(-1), d_tid(-1), s_tid(-1), alpha_tid(-1), bump_tid(-1), skip(0), is_used(0) {}
	bool add_poly(vntc_vect_t const &poly);
	void mark_as_used() {is_used = 1;}
	bool mat_is_used() const {return is_used;}
	int get_render_texture() const {return d_tid;}
	bool is_partial_transparent() const {return (alpha < 1.0 || alpha_tid >= 0);}
	void render(texture_manager const &tmgr, int default_tid, bool is_shadow_pass);
	colorRGBA get_ad_color() const;
	colorRGBA get_avg_color(texture_manager const &tmgr, int default_tid=-1) const;
};


class model3d {

	// geometry
	geometry_t unbound_geom;
	int unbound_tid;
	colorRGBA unbound_color;
	vector<polygon_t> split_polygons_buffer;

	// materials
	deque<material_t> materials;
	string_map_t mat_map; // maps material names to materials indexes
	set<string> undef_materials; // to reduce warning messages

public:
	// textures
	texture_manager &tmgr;

	model3d(texture_manager &tmgr_, int def_tid=-1, colorRGBA const &def_c=WHITE)
		: tmgr(tmgr_), unbound_tid((def_tid >= 0) ? def_tid : WHITE_TEX), unbound_color(def_c) {}
	unsigned num_materials(void) const {return materials.size();}

	material_t &get_material(int mat_id) {
		assert(mat_id >= 0 && (unsigned)mat_id < materials.size());
		return materials[mat_id];
	}

	// creation and query
	void add_polygon(vntc_vect_t const &poly, int mat_id, vector<polygon_t> *ppts=NULL);
	int get_material_ix(string const &material_name, string const &fn);
	int find_material(string const &material_name);
	void mark_mat_as_used(int mat_id);
	void remove_excess_cap();
	void clear();
	void free_context();
	void load_all_used_tids();
	void bind_all_used_tids();
	void render(bool is_shadow_pass); // const?
};


struct model3ds : public deque<model3d> {

	texture_manager tmgr;

	void clear();
	void free_context();
	void render(bool is_shadow_pass); // const?
};


class coll_obj; // forward declaration
void copy_polygon_to_cobj(polygon_t const &poly, coll_obj &cobj);

bool split_polygon(polygon_t const &poly, vector<polygon_t> &ppts);

void free_model_context();
void render_models(bool shadow_pass);

bool read_object_file(char *filename, vector<polygon_t> *ppts, geom_xform_t const &xf,
	int def_tid, colorRGBA const &def_c, bool load_model_file, bool verbose);


#endif // _MODEL3D_H_
