#ifndef __GEOM_H__
#define __GEOM_H__
#include <vector>
#include <memory>
#include <iostream>
#include "algebra/vec.h"
#include "algebra/mat.h"
#include "syntactic-sugar/assert.h"

namespace UM {

	inline double unsigned_area(const vec3 &A, const vec3 &B, const vec3 &C) {
		return 0.5*cross(B-A, C-A).norm();
	}

	// Signed volume for a tet with vertices (A,B,C,D) such that (AB, AC, AD) form a right hand basis
	inline double tet_volume(const vec3 &A, const vec3 &B, const vec3 &C, const vec3 &D) {
		return ((B-A)*cross(C-A,D-A))/6.;
	}

	inline vec3 bary_verts(const vec3 v[], const int nbv) {
		vec3 ave{0, 0, 0};
		for (int lv=0; lv<nbv; lv++)
			ave += v[lv];
		return ave / static_cast<double>(nbv);
	}

	inline vec3 normal(const vec3 v[], const int nbv) {
		vec3 res{0, 0, 0};
		vec3 bary = bary_verts(v, nbv);
		
		for (int lv=0; lv<nbv; lv++)
			res += cross(
				v[lv]-bary,
				v[(lv+1)%nbv]-bary
			);
		return res.normalized();
	}

	inline double unsigned_area(const vec3 v[], const int nbv) {
		double a = 0;
		vec3 G = UM::bary_verts(v, nbv);
		for (int lv=0; lv<nbv; lv++) {
			const vec3 &A = v[lv];
			const vec3 &B = v[(lv+1)%nbv];
			a += UM::unsigned_area(G, A, B);
		}
		return a;
	}   

	struct Segment3;

	struct Segment2 {
		Segment2(vec2 x,vec2 y){v[0]=x;v[1]=y;}

		vec2 v[2]{};

		inline vec2 vector();
		inline double length2() const;
		inline double length() const;
		inline Segment3 xy0() const;

        double distance(const vec2 &p) const;

		inline vec2& operator[](int i) { return v[i]; }
		inline vec2 operator[](int i)const { return v[i]; }
	};

	struct Segment3 {
		Segment3(vec3 x,vec3 y){v[0]=x;v[1]=y;}

		vec3 v[2]{};

		inline vec3 vector();
		inline double length2() const;
		inline double length() const;
		inline Segment2 xy() const;
		inline double bary_coords(const vec3 &P) const;
		inline vec3 closest_point(const vec3 &P) const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i) const { return v[i]; }
	};

	struct Triangle3;

	struct Triangle2 {
		Triangle2(vec2 x,vec2 y, vec2 z){v[0]=x;v[1]=y;v[2]=z;}

		vec2 v[3]{};

		inline vec2 bary_verts() const;
		inline double signed_area() const;
		inline Triangle3 xy0() const;
		Triangle2 dilate(double scale) const;
		vec3 bary_coords(vec2 G) const;
		mat<2,3> grad_operator() const;
		mat<2,3> grad_operator2() const;
		vec2 grad(vec3 u) const;

		inline vec2& operator[](int i) { return v[i]; }
		inline vec2 operator[](int i) const { return v[i]; }
	};

	struct Triangle3 {
		Triangle3(vec3 x,vec3 y, vec3 z){v[0]=x;v[1]=y;v[2]=z;}
		vec3 v[3]{};

		inline vec3 cross_product() const;
		inline vec3 normal() const;
		inline vec3 bary_verts() const;
		vec3 bary_coords(vec3 G) const;
		inline double unsigned_area() const;
		Triangle2 project() const;
		inline Triangle2 xy() const;
		Triangle3 dilate(double scale) const;
		mat3x3 tangent_basis() const;
		mat3x3 tangent_basis(vec3 first_axis) const;
		mat3x3 grad_operator() const;
		mat3x3 grad_operator2() const;
		vec3 grad(vec3 u) const;
		inline mat3x3 as_matrix() const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i)const { return v[i]; }
	};

	struct Quad3;

	struct Quad2 {
		Quad2(vec2 x,vec2 y, vec2 z,vec2 w){v[0]=x;v[1]=y;v[2]=z;v[3]=w;}

		vec2 v[4] = {};

		inline vec2 bary_verts() const;
		double signed_area() const;
		inline Quad3 xy0() const;
		double scaled_jacobian() const;

		inline vec2& operator[](int i) { return v[i]; }
		inline vec2 operator[](int i)const { return v[i]; }
	};

	struct Quad3 {
		Quad3(vec3 x,vec3 y, vec3 z,vec3 w){v[0]=x;v[1]=y;v[2]=z;v[3]=w;}
		vec3 v[4] = {};

		vec3 normal() const;
		inline vec3 bary_verts() const;
		double unsigned_area() const;
		inline Quad2 xy() const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i) const { return v[i]; }
	};

	struct Poly3 {
		std::vector<vec3> v = {};

		vec3 normal() const;
		vec3 bary_verts() const;
		double unsigned_area() const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i) const { return v[i]; }
	};

	struct Tetrahedron {
		Tetrahedron(vec3 x,vec3 y, vec3 z,vec3 w){v[0]=x;v[1]=y;v[2]=z;v[3]=w;}
		vec3 v[4] = {};

		vec3 bary_verts() const;
		inline double volume() const;
		vec4 bary_coords(vec3 G) const;
		mat<3,4> grad_operator() const;
		vec3 grad(vec4 u) const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i) const { return v[i]; }
	};

	struct Hexahedron {
		Hexahedron (vec3 a,vec3 b, vec3 c,vec3 d,vec3 e,vec3 f, vec3 g,vec3 h){v[0]=a;v[1]=b;v[2]=c;v[3]=d;v[4]=e;v[5]=f;v[6]=g;v[7]=h;}
		vec3 v[8] = {};

		double volume() const;
		vec3 bary_verts() const;
		double scaled_jacobian() const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i) const { return v[i]; }
	};

	struct Wedge {
		Wedge(vec3 a, vec3 b, vec3 c, vec3 d, vec3 e, vec3 f) { v[0]=a;v[1]=b;v[2]=c;v[3]=d;v[4]=e;v[5]=f; }
		vec3 v[6] = {};

		double volume() const;
		vec3 bary_verts() const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i) const { return v[i]; }
	};

	struct Pyramid {
		Pyramid (vec3 a,vec3 b, vec3 c,vec3 d,vec3 e){v[0]=a;v[1]=b;v[2]=c;v[3]=d;v[4]=e;}
		vec3 v[5] = {};

		double volume() const;
		vec3 bary_verts() const;
		inline Quad3 base() const;
		inline vec3 apex() const;

		inline vec3& operator[](int i) { return v[i]; }
		inline vec3 operator[](int i) const { return v[i]; }
	};

	inline vec2 Segment2::vector() { return v[1] - v[0]; }
	inline double Segment2::length2() const { return (v[1] - v[0]).norm2(); }
	inline double Segment2::length() const { return (v[1] - v[0]).norm(); }
	inline Segment3 Segment2::xy0() const { return Segment3(vec3(v[0].x, v[0].y, 0),vec3(v[1].x, v[1].y, 0)); }

	inline vec3 Segment3::vector() { return v[1] - v[0]; }
	inline double Segment3::length2() const { return (v[1] - v[0]).norm2(); }
	inline double Segment3::length() const { return (v[1] - v[0]).norm(); }
	inline Segment2 Segment3::xy() const { return Segment2 (vec2(v[0].x, v[0].y),vec2( v[1].x, v[1].y)); }

	inline double Segment3::bary_coords(const vec3 &P) const {
		if (length2() < 1e-15) return 0.5;
		return (v[1] - P) * (v[1] - v[0]) / (v[1] - v[0]).norm2();
	}

	inline vec3 Segment3::closest_point(const vec3 &P) const {
		double c = bary_coords(P);
		if (c < 0) return v[1];
		if (c > 1) return v[0];
		return c * v[0] + (1. - c) * v[1];
	}


	inline std::ostream& operator<<(std::ostream& out, const Segment2& s) {
		for (int i=0; i<2; i++) out << s[i] << std::endl;
		return out;
	}

	inline std::ostream& operator<<(std::ostream& out, const Segment3& s) {
		for (int i=0; i<2; i++) out << s[i] << std::endl;
		return out;
	}

	inline vec2 Triangle2::bary_verts() const {
		return (v[0] + v[1] + v[2]) / 3;
	}

	inline double Triangle2::signed_area() const {
		const vec2 &A = v[0];
		const vec2 &B = v[1];
		const vec2 &C = v[2];
		return .5*((B.y-A.y)*(B.x+A.x) + (C.y-B.y)*(C.x+B.x) + (A.y-C.y)*(A.x+C.x));
	}

	inline Triangle3 Triangle2::xy0() const {
		return Triangle3(v[0].xy0(), v[1].xy0(), v[2].xy0());
	}

	inline vec3 Triangle3::cross_product() const {
		return cross(v[1] - v[0], v[2] - v[0]);
	}

	inline vec3 Triangle3::normal() const {
		return cross_product().normalized();
	}

	inline vec3 Triangle3::bary_verts() const {
		return (v[0] + v[1] + v[2]) / 3;
	}

	inline double Triangle3::unsigned_area() const {
		return UM::unsigned_area(v[0], v[1], v[2]);
	}

	inline Triangle2 Triangle3::xy() const {
		return Triangle2(v[0].xy(), v[1].xy(), v[2].xy());
	}

	inline mat3x3 Triangle3::as_matrix() const {
		return {{v[0], v[1], v[2]}};
	}

	inline vec2 Quad2::bary_verts() const {
		return (v[0] + v[1] + v[2] + v[3]) / 4;
	}

	inline double Quad2::signed_area() const {
		const vec2 A = v[2] - v[0];
		const vec2 B = v[3] - v[1];
		return (A.x * B.y - A.y * B.x) * .5;
	}

	inline Quad3 Quad2::xy0() const {
		return Quad3(v[0].xy0(), v[1].xy0(), v[2].xy0(), v[3].xy0());
	}

	inline Quad2 Quad3::xy() const {
		return Quad2(v[0].xy(), v[1].xy(), v[2].xy(), v[3].xy());
	}

	inline vec3 Quad3::bary_verts() const {
		return (v[0] + v[1] + v[2] + v[3]) / 4;
	}

	inline double Tetrahedron::volume() const {
		return tet_volume(v[0], v[1], v[2], v[3]);
	}

	inline Quad3 Pyramid::base() const {
		return Quad3(v[0], v[1], v[2], v[3]);
	}

	inline vec3 Pyramid::apex() const {
		return v[4];
	}

}

#endif //__GEOM_H__
