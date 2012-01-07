// 3D World - physics ojbects classes
// by Frank Gennari
// 7/23/06

#ifndef _PHYSICS_OBJECTS_H_
#define _PHYSICS_OBJECTS_H_


#include "3DWorld.h"
#include "collision_detect.h"

float const MAX_PART_CLOUD_RAD = 0.25;


struct spark_t {

	float s;
	point pos;
	colorRGBA c;
	static int  const status = 1;
	static float const radius;
	point const &get_pos() const {return pos;}

	spark_t() {}
	spark_t(point const &p_, colorRGBA const &c_, float s_) : pos(p_), c(c_), s(s_) {}
	void draw() const;
};


struct star { // size = 32

	float intensity;
	colorRGBA color;
	point pos;
};


struct obj_type { // size = 88

	int lifetime, tid;
	unsigned flags;
	float mass, radius, volume, surface_area, density, air_factor, terminal_vel, friction_factor;
	float health, damage, min_t, max_t, elasticity, gravity, deform, def_recover;
	colorRGBA color;
};


struct basic_physics_obj { // size = 20

	point pos;
	int time;
	char status;

	basic_physics_obj(point const &pos_=all_zeros, int status_=0) : pos(pos_), time(0), status(status_) {}
	void init(point const &p);
	void init_gen_rand(point const &p, float rxy, float rz);
	point const &get_pos() const {return pos;}

	bool operator<(basic_physics_obj const &o) const {
		if (status == 0 && o.status != 0) return 0;
		if (status != 0 && o.status == 0) return 1;
		return (o.time < time);
	}
};


struct bubble : public basic_physics_obj { // size = 44

	float radius, velocity;
	colorRGBA color;

	void gen(point const &p, float r=0.0, colorRGBA const &c=WATER_C);
	void draw() const;
	void apply_physics(unsigned i);
};


struct particle_cloud : public basic_physics_obj { // size = 88

	struct part : public sphere_t {
		bool status;
	};
	bool acc_smoke, no_lighting, red_only;
	int source, damage_type;
	float radius, init_radius, density, darkness, damage;
	vector3d init_vel;
	colorRGBA base_color;
	vector<part> parts;

	void gen(point const &p, colorRGBA const &bc, vector3d const &iv, float r,
		float den, float dark, float dam, int src, int dt, bool as, bool use_parts=1, bool nl=0);
	void draw() const;
	void draw_part(point const &p, float r, colorRGBA c) const;
	void apply_physics(unsigned i);
	float get_rscale() const {return CLIP_TO_01(1.0f - (radius - init_radius)/(MAX_PART_CLOUD_RAD - init_radius));}
	bool is_fire()     const {return (damage_type == BURNED || damage_type == FIRE);}
};


struct fire : public basic_physics_obj { // size = 60

	int source;
	bool is_static;
	float radius, heat, cval, inten, light_bwidth;
	vector3d velocity;

	void gen(point const &p, float size, float intensity, int src, bool is_static_, float light_bw);
	void set_fire_color() const;
	void draw() const;
	void apply_physics(unsigned i);
	void extinguish();
};


struct decal_obj : public basic_physics_obj { // size = 76

	bool is_glass;
	int cid;
	float radius, alpha;
	colorRGBA color;
	point ipos;
	vector3d orient;

	decal_obj() : is_glass(0), cid(-1), radius(0.0), alpha(1.0), color(BLACK) {}
	void gen(point const &p, float r, vector3d const &o, int cid_=-1, float init_alpha=1.0, colorRGBA const &color_=BLACK, bool is_glass_=0);
	void draw() const;
	bool is_on_cobj(int cobj) const;
	void check_cobj();
	void apply_physics(unsigned i);
	float get_alpha() const;
	vector3d get_platform_delta() const;
	point const get_pos() const {return (pos + get_platform_delta());}
};


struct dwobject : public basic_physics_obj { // size = 67(68) (dynamic world object)

	int coll_id;
	short type, source, flags, shadow;
	unsigned char direction;
	float health, angle;
	vector3d velocity, orientation, init_dir, vdeform;

	dwobject() : coll_id(-1) {}
	dwobject(int type_, point const &pos_, vector3d const &vel_=all_zeros, int status_=0, float health_=0.0)
		: basic_physics_obj(pos_, status_), coll_id(-1), type(type_), source(-2), flags(0), shadow(0),
		direction(0), health(health_), angle(0.0), velocity(vel_), orientation(0.0, 0.0, -1.0),
		init_dir(0.0, 0.0, -1.0), vdeform(all_zeros) {}
	float get_true_radius() const;
	void advance_object(bool disable_motionless_objects, int iter, int obj_index);
	int surface_advance();
	void set_orient_for_coll(vector3d const *const forced_norm);
	int check_water_collision(float vz_old);
	void surf_collide_obj() const;
	void elastic_collision(point const &obj_pos, float energy, int obj_type);
	int object_bounce(int coll_type, vector3d &norm, float elasticity2, float zval, float z_offset, vector3d const &obj_vel=zero_vector);
	int object_still_stopped(int obj_index);
	void do_coll_damage();
	int check_vert_collision(int obj_index, int do_coll_funcs, int iter, vector3d *cnorm=NULL,
		vector3d const &mdir=all_zeros, bool skip_dynamic=0, bool only_drawn=0, int only_cobj=-1);
	int multistep_coll(point const &last_pos, int obj_index, unsigned nsteps);
	void update_vel_from_damage(vector3d const &dv);
	void damage_object(float damage, point const &dpos, point const &shoot_pos, int weapon);
	void update_precip_type();
	void add_obj_dynamic_light(int index) const;
	bool disabled() const {return (status == OBJ_STAT_BAD || status == OBJ_STAT_RES);}
	void disable() {status = 0; time = 0; health = 0.0;} // set all three just in case
	void verify_data() const {if (is_nan(pos) || is_nan(velocity)) print_and_terminate();}
	void print_and_terminate() const;
	bool lm_coll_invalid() const;
	bool invalid_coll(coll_obj const &cobj) const {return (type == LANDMINE && lm_coll_invalid() && cobj.is_player());}
	bool proc_stuck(bool static_top_coll);
};


class vert_coll_detector {

	dwobject &obj;
	int type, iter;
	bool player, already_bounced, skip_dynamic, only_drawn;
	int coll, obj_index, do_coll_funcs, only_cobj;
	unsigned cdir, lcoll;
	float z_old, o_radius, z1, z2;
	point pos, pold;
	vector3d motion_dir, obj_vel;
	vector3d *cnorm;
	dwobject temp;

	bool safe_norm_div(float rad, float radius, vector3d &norm);
	void check_cobj_intersect(int index, bool enable_cfs, bool player_step);
	void init_reset_pos();
public:
	vert_coll_detector(dwobject &obj_, int obj_index_, int do_coll_funcs_, int iter_, vector3d *cnorm_,
		vector3d const &mdir=zero_vector, bool skip_dynamic_=0, bool only_drawn_=0, int only_cobj_=-1) :
	obj(obj_), type(obj.type), iter(iter_), player(type == CAMERA || type == SMILEY || type == WAYPOINT),
	already_bounced(0), skip_dynamic(skip_dynamic_), only_drawn(only_drawn_), coll(0), obj_index(obj_index_),
	do_coll_funcs(do_coll_funcs_), only_cobj(only_cobj_), cdir(0), lcoll(0), z_old(obj.pos.z), cnorm(cnorm_),
	pos(obj.pos), pold(obj.pos), motion_dir(mdir), obj_vel(obj.velocity) {}

	void check_cobj(int index);
	int check_coll();
};


struct enabled_pos {

	point pos;
	bool enabled;

	enabled_pos() : enabled(0) {}
	enabled_pos(point const &pos_, bool enabled_=1) : pos(pos_), enabled(enabled_) {}
};


template<typename T> class obj_vector_t : public vector<T> {

	unsigned cur_avail;

	struct int_uint_pair {
		int i;
		unsigned u;
		int_uint_pair(int i_=0, unsigned u_=0) : i(i_), u(u_) {}
		bool operator<(int_uint_pair const &p) const {return (i > p.i);} // greater than on the int
	};

	void inc_cur_avail() {
		++cur_avail;
		if (cur_avail == size()) cur_avail = 0;
	}

public:
	obj_vector_t(unsigned sz=0) : vector<T>(sz), cur_avail(0) {}

	unsigned choose_element(bool peek=0) {
		assert(!empty());
		assert(cur_avail < size());
		vector<T> const &v(*this);
		unsigned const start(cur_avail), sz(size());

		for (unsigned i = 0; i < sz; ++i) {
			unsigned ix(i + start);
			if (ix >= sz) ix -= sz;
			if (v[ix].status == 0) {cur_avail = ix; break;}
			if (v[ix].time > v[cur_avail].time) cur_avail = ix; // replace oldest element
		}
		unsigned const chosen(cur_avail);
		assert(chosen < size());
		if (!peek) inc_cur_avail();
		return chosen;
	}

	void choose_elements(vector<unsigned> &ixs, unsigned num) {
		assert(num <= size());
		assert(cur_avail < size());
		ixs.clear();
		if (num == 0) return;
		ixs.reserve(num);
		vector<T> const &v(*this);
		unsigned const start(cur_avail), sz(size());

		for (unsigned i = 0; i < sz; ++i) {
			unsigned ix(i + start);
			if (ix >= sz) ix -= sz;
			if (v[ix].status != 0) continue; // used element
			ixs.push_back(ix);
			
			if (ixs.size() == num) {
				cur_avail = ix;
				inc_cur_avail();
				return; // we're done
			}
		}
		unsigned const num_rem(num - ixs.size());
		vector<int_uint_pair> time_ixs;
		time_ixs.reserve(sz);

		for (unsigned i = 0; i < sz; ++i) {
			if (v[i].status != 0) time_ixs.push_back(int_uint_pair(v[i].time, i));
		}
		assert(num_rem <= time_ixs.size());
		sort(time_ixs.begin(), time_ixs.end()); // sort largest to smallest by time
		
		for (unsigned i = 0; i < num_rem; ++i) {
			ixs.push_back(time_ixs[i].u);
		}
	}
};


class cloud_manager_t : public obj_vector_t<particle_cloud> {

	unsigned cloud_tid, txsize, tysize;
	void set_red_only(bool val) {for (iterator i = begin(); i != end(); ++i) i->red_only = val;}

public:
	cloud_manager_t() : cloud_tid(0), txsize(0), tysize(0) {}
	~cloud_manager_t() {free_textures();}
	void create_clouds();
	void update_lighting();
	cube_t get_bcube() const;
	bool create_texture(bool force_recreate);
	void free_textures();
	void draw();
};


struct transform_data; // forward reference


struct predef_obj { // size = 28

	point pos;
	int type, obj_used;
	float regen_time, cur_time;

	predef_obj(point const &pos_=all_zeros, int type_=0, float rtime=0)
		: pos(pos_), type(type_), obj_used(-1), regen_time(rtime), cur_time(0) {}
};


class obj_group { // size = 36

	obj_vector_t<dwobject> objects;
	vector<predef_obj> predef_objs;
	transform_data *td;

public:
	unsigned init_objects, max_objs, app_rate, end_id, new_id;
	bool enabled, reorderable;
	short type;
	unsigned char flags;

	obj_group() : td(NULL) {}
	void create(int obj_type_, unsigned max_objects_, unsigned init_objects_,
		unsigned app_rate_, bool init_enabled_, bool reorderable_, bool auto_max);
	unsigned get_updated_max_objs() const;
	void update_app_rate(float const val, unsigned min_app, unsigned max_app);
	void init_group();
	void preproc_this_frame();
	void remove_reset_cobjs();
	unsigned max_objects() const {return objects.size();}
	int choose_object(bool peek=0);
	void create_object_at(unsigned i, point const &pos);
	void enable();
	void disable();
	bool is_enabled()   const {return enabled;}
	bool large_radius() const;
	void set_enable(bool enabled_) {if (enabled_) enable(); else disable();}
	void toggle_enable() {set_enable(!enabled);}
	void free();
	void shift(vector3d const &vd);
	transform_data *get_td() {assert(td); return td;}
	dwobject       &get_obj(unsigned i)       {assert(enabled && i < objects.size()); return objects[i];}
	dwobject const &get_obj(unsigned i) const {assert(enabled && i < objects.size()); return objects[i];}
	bool obj_within_dist(unsigned i, point const &pos, float dist) const;
	bool temperature_ok() const;
	bool obj_has_shadow(unsigned obj_id) const;
	int get_ptype() const;
	void add_predef_obj(point const &pos, int type, int rtime);
	int get_next_predef_obj(dwobject &obj, unsigned ix);
	vector<predef_obj> const &get_predef_objs() const {return predef_objs;}
};


template<typename T> void reset_status(vector<T> &objs) {

	for (unsigned i = 0; i < objs.size(); ++i) {
		objs[i].status = 0;
	}
}


// group flag bits
#define WAS_ADVANCED     0x02
#define JUST_INIT        0x01
#define PRECIPITATION    0x04
#define APP_FROM_LT      0x08

// object type flag bits
#define SEMI_TRANSPARENT 0x01
#define BLEND            0x02
#define SPECULAR         0x04
#define LOW_SPECULAR     0x08
#define SELECTABLE       0x10
#define NO_FALL          0x20
#define FALL_EVERYWHERE  0x40
#define TAIL_WHEN_FALL   0x80
#define OBJ_IS_DROP      0x100
#define OBJ_ROLLS        0x200
#define DEFORMABLE       0x400
#define VERTEX_DEFORM    0x800
#define OBJ_EXPLODES     0x1000
#define EXPL_ON_COLL     0x2000
#define COLL_DESTROYS    0x4000
#define IS_PRECIP        0x8000
#define NO_WATER_DAMAGE  0x10000
#define OBJ_IS_FLAT      0x20000
#define OBJ_IS_CYLIN     0x40000

// object flag bits
#define XY_STOPPED       0x01
//      0x02
#define Z_STOPPED        0x04
#define FLOATING         0x08
#define UNDERWATER       0x10
#define CAMERA_VIEW      0x20
#define IN_WATER         0x40
#define SHADOWED         0x80

// object flags (second byte)
#define TYPE_FLAG        0x0100
#define IS_ON_ICE        0x0200
#define STATIC_COBJ_COLL 0x0400
#define OBJ_COLLIDED     0x0800
#define PLATFORM_COLL    0x1000
#define USER_PLACED      0x2000
#define WAS_PUSHED       0x4000
//#define x 0x8000

unsigned const XYZ_STOPPED(XY_STOPPED | Z_STOPPED);
unsigned const ALL_COLL_STOPPED(XYZ_STOPPED | STATIC_COBJ_COLL);


#endif // _PHYSICS_OBJECTS_H_

