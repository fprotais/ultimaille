#include <iostream>
#include <algorithm>
#include <cassert>
#include "attributes.h"
#include "surface.h"
#include "volume.h"
#include "attr_binding.h"
#include "surface_connectivity.h"

namespace UM {

    Surface::Connectivity::Connectivity(Surface &m) : v2c(m, -1), c2f(m, -1), c2c(m, -1), active(m, true) {
    }

    void Surface::connect() {
        if (!conn) conn = std::make_unique<Connectivity>(*this);
        conn->active.fill(true);
        conn->c2f.fill(-1);
        conn->c2c.fill(-1);
        conn->v2c.fill(-1);

        for (int f = 0; f < nfacets(); f++)
            for (int fc = 0; fc < facet_size(f); fc++) {
                int c = corner(f, fc);
                int v = vert(f, fc);
                conn->c2f[c] = f;
                conn->v2c[v] = c;
            }
        for (int f = 0; f < nfacets(); f++) // if it ain't broken, don't fix it
            for (int fc = 0; fc < facet_size(f); fc++) {
                int c = corner(f, fc);
                int v = vert(f, fc);
                conn->c2c[c] = conn->v2c[v];
                conn->v2c[v] = c;
            }
    }

    void Surface::disconnect() {
        conn.reset();
    }

    void Surface::compact(bool delete_isolated_vertices) {
        if (!conn) return;
        um_assert(conn->active.ptr!=nullptr);
        std::vector<bool> to_kill = conn->active.ptr->data;
        to_kill.flip();
        disconnect(); // happy assert(conn==nullptr)!
        delete_facets(to_kill);
        if (delete_isolated_vertices)
            Surface::delete_isolated_vertices();
        connect();
    }

    // unsigned area for a 3D triangle
    inline double unsigned_area(const vec3 &A, const vec3 &B, const vec3 &C) {
        return 0.5*cross(B-A, C-A).norm();
    }

    vec3 Surface::Util::bary_verts(const int f) const {
        vec3 ave = {0, 0, 0};
        const int nbv = m.facet_size(f);
        for (int lv=0; lv<nbv; lv++)
            ave += m.points[m.vert(f, lv)];
        return ave / static_cast<double>(nbv);
    }

    void Surface::delete_isolated_vertices()  {
        std::vector<bool> to_kill(nverts(), true);
        for (int f=0; f<nfacets(); f++)
            for (int lv=0; lv<facet_size(f); lv++)
                to_kill[vert(f, lv)] = false;
        delete_vertices(to_kill);
    }

    double Triangles::Util::unsigned_area(const int f) const {
        const vec3 &A = m.points[m.vert(f, 0)];
        const vec3 &B = m.points[m.vert(f, 1)];
        const vec3 &C = m.points[m.vert(f, 2)];
        return UM::unsigned_area(A, B, C);
    }

    void Triangles::Util::project(const int f, vec2& z0, vec2& z1, vec2& z2) const {
        const vec3 &A = m.points[m.vert(f, 0)];
        const vec3 &B = m.points[m.vert(f, 1)];
        const vec3 &C = m.points[m.vert(f, 2)];

        vec3 X = (B - A).normalized(); // construct an orthonormal 3d basis
        vec3 Z = cross(X, C - A).normalized();
        vec3 Y = cross(Z, X);

        z0 = vec2(0,0); // project the triangle to the 2d basis (X,Y)
        z1 = vec2((B - A).norm(), 0);
        z2 = vec2((C - A)*X, (C - A)*Y);
    }

    vec3 Triangles::Util::normal(const int f) const {
        const vec3 &A = m.points[m.vert(f, 0)];
        const vec3 &B = m.points[m.vert(f, 1)];
        const vec3 &C = m.points[m.vert(f, 2)];
        return cross(B-A, C-A).normalized();
    }

    double Quads::Util::unsigned_area(const int f) const {
        double a = 0;
        vec3 G = m.util.bary_verts(f);
        for (int lv=0; lv<4; lv++) {
            const vec3 &A = m.points[m.vert(f,  lv     )];
            const vec3 &B = m.points[m.vert(f, (lv+1)%4)];
            a += UM::unsigned_area(G, A, B);
        }
        return a;
    }

    vec3 Quads::Util::normal(const int f) const {
        vec3 res = {0, 0, 0};
        vec3 bary = m.util.bary_verts(f);
        for (int lv=0; lv<4; lv++)
            res += cross(
                    m.points[m.vert(f,  lv     )]-bary,
                    m.points[m.vert(f, (lv+1)%4)]-bary
                    );
        return res.normalized();
    }

    void Surface::resize_attrs() {
        for (auto &wp : attr_facets)  if (auto spt = wp.lock())
            spt->resize(nfacets());
        for (auto &wp : attr_corners) if (auto spt = wp.lock())
            spt->resize(ncorners());
    }

    void Surface::compress_attrs(const std::vector<bool> &facets_to_kill) {
        assert(facets_to_kill.size()==(size_t)nfacets());
        std::vector<int>  facets_old2new(nfacets(),  -1);
        std::vector<int> corners_old2new(ncorners(), -1);

        int new_nb_facets  = 0;
        int new_nb_corners = 0;

        for (int f=0; f<nfacets(); f++) {
            if (facets_to_kill[f]) continue;
            for (int lv=0; lv<facet_size(f); lv++)
                corners_old2new[corner(f, lv)] = new_nb_corners++;
            facets_old2new[f] = new_nb_facets++;
        }

        std::erase_if(attr_facets,  [](std::weak_ptr<GenericAttributeContainer> ptr) { return ptr.lock()==nullptr; }); // remove dead attributes
        std::erase_if(attr_corners, [](std::weak_ptr<GenericAttributeContainer> ptr) { return ptr.lock()==nullptr; });
        for (auto &wp : attr_facets) { // compress attributes
            auto spt = wp.lock();
            assert(spt!=nullptr);
            spt->compress(facets_old2new);
        }
        for (auto &wp : attr_corners) {
            auto spt = wp.lock();
            assert(spt!=nullptr);
            spt->compress(corners_old2new);
        }
    }

    void Surface::delete_vertices(const std::vector<bool> &to_kill) {
        assert(to_kill.size()==(size_t)nverts());
        std::vector<int> old2new;
        points.delete_points(to_kill, old2new); // conn.v2c is a PointAttribute, it is automatically updated here
        for (int &v : facets) {
            assert(old2new[v]>=0);
            v = old2new[v];
        }
    }

    void Surface::delete_facets(const std::vector<bool> &to_kill) {
        assert(!connected());
        assert(to_kill.size()==(size_t)nfacets());
        compress_attrs(to_kill);  // TODO: if to_kill comes from an attribute, compressing the attribute compromises the code below
        int new_nb_corners = 0;
        for (int f=0; f<nfacets(); f++) {
            if (to_kill[f]) continue;
            for (int lv=0; lv<facet_size(f); lv++)
                facets[new_nb_corners++] = vert(f, lv);
        }
        facets.resize(new_nb_corners);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    int Triangles::create_facets(const int n) {
        assert(!connected());
        facets.resize(facets.size()+n*3);
        resize_attrs();
        return nfacets()-n;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    int Quads::create_facets(const int n) {
        assert(!connected());
        facets.resize(facets.size()+n*4);
        resize_attrs();
        return nfacets()-n;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////

    int Polygons::create_facets(const int n, const int size) {
        assert(!connected());
        for (int i=0; i<n*size; i++)
            facets.push_back(0);
        for (int i=0; i<n; i++)
            offset.push_back(offset.back()+size);
        resize_attrs();
        return nfacets()-n;
    }

    void Polygons::delete_facets(const std::vector<bool> &to_kill) {
        assert(!connected());
        Surface::delete_facets(to_kill); // TODO: if to_kill comes from an attribute, Surface::delete_facets compacts it, thus compromising the code below
        int new_nb_facets = 0;
        for (int f=0; f<nfacets(); f++) {
            if (to_kill[f]) continue;
            offset[new_nb_facets+1] = offset[new_nb_facets] + facet_size(f);
            ++new_nb_facets;
        }
        offset.resize(new_nb_facets+1);
    }
}

