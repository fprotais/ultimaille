#ifndef __VOLUME_CONNECTIVITY_H__
#define __VOLUME_CONNECTIVITY_H__

#include <vector>
#include <memory>
#include "syntactic-sugar/assert.h"
#include "volume.h"

namespace UM {
    struct Volume;

    struct CellsAdjacency {
        CellsAdjacency(const Volume &m);

        int opposite_c(const int he) const;
        const Volume &m;
        std::vector<int> adjacent; // adjacency between half-facets
    };

    struct HalfEdgeHelper { // half-edge-like connectivity interface
        HalfEdgeHelper(const Volume &mesh) : m(mesh) {}

        int nhalfedges() const;
        int nhalfedges_per_cell() const;

        vec3          geom(const int he) const;
        int           cell(const int he) const;
        int          facet(const int he) const;
        int         corner(const int he) const;
        int     cell_facet(const int he) const;
        int  cell_halfedge(const int he) const;
        int facet_halfedge(const int he) const;
        int           from(const int he) const;
        int             to(const int he) const;
        int           prev(const int he) const;
        int           next(const int he) const;

        int opposite_f(const int he) const;
        int opposite_c(const CellsAdjacency &adj, const int he) const;
        const Volume &m;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  DEPRECATED DEPRECATED DEPRECATED DEPRECATED DEPRECATED DEPRECATED DEPRECATED DEPRECATED DEPRECATED DEPRECATED DEPRECATED  //
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct [[deprecated]] VolumeConnectivity { // half-edge-like connectivity interface
        VolumeConnectivity(const Volume &p_m);

        int halfedge(const int cell, const int cell_facet, const int facet_he) const;
        int halfedge_from_verts(const int c, const int org, const int dst) const;

        int           cell(const int he) const;
        int          facet(const int he) const; // TODO: facet -> halffacet?
        int     cell_facet(const int he) const;
        int facet_halfedge(const int he) const; // TODO find better name
        int     facet_size(const int he) const;
        int           from(const int he) const;
        int             to(const int he) const;
        int           prev(const int he) const;
        int           next(const int he) const;

        int opposite_c(const int he) const; // TODO speed-up the implementation
        int opposite_f(const int he) const; // of these two functions

        std::vector<int> halfedges_around_edge(const int he) const;

        static constexpr int max_f = 6; // max number of faces per cell
        static constexpr int max_h = 4; // max number of halfedges per face
        const Volume &m;
        std::vector<int> adjacent; // adjacency between half-facets
    };
}

#endif //__VOLUME_CONNECTIVITY_H__

