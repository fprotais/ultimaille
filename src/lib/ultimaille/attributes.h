#ifndef __ATTRIBUTES_H__
#define __ATTRIBUTES_H__
#include <vector>
#include <memory>
#include "surface.h"
#include "pointset.h"
#include "permutation.h"

struct GenericAttributeContainer {
    virtual void resize(const int n) = 0;
    virtual void compress(const std::vector<int> &old2new) = 0;
    virtual ~GenericAttributeContainer() = default;
};

template <typename T> struct AttributeContainer : GenericAttributeContainer {
    AttributeContainer(const int n) : data(n) {}
    void resize(const int n) { data.resize(n); }
    void compress(const std::vector<int> &old2new) { // NB: old2new is not a permutation!
        assert(old2new.size()==data.size());
        int cnt = 0;
        for (int i=0; i<static_cast<int>(old2new.size()); i++) {
            if (old2new[i]<0) continue;
            data[old2new[i]] = data[i];
            cnt++;
        }
        resize(cnt);
    }
    std::vector<T> data;
};

template <typename T> struct GenericAttribute {
    GenericAttribute(int size) : ptr(new AttributeContainer<T>(size)) {}
    T& operator[](const int i)       { return std::dynamic_pointer_cast<AttributeContainer<T> >(ptr)->data[i]; }
    T  operator[](const int i) const { return std::dynamic_pointer_cast<AttributeContainer<T> >(ptr)->data[i]; }
    std::shared_ptr<GenericAttributeContainer> ptr;
};

template <typename T> struct PointAttribute : GenericAttribute<T> {
    PointAttribute(PointSet &p) : GenericAttribute<T>(p.size())  {
        p.attr.push_back(this->ptr);
    }
};

template <typename T> struct FacetAttribute : GenericAttribute<T> {
    FacetAttribute(Surface &m) : GenericAttribute<T>(m.nfacets())  {
        m.attr_facets.push_back(this->ptr);
    }
};

template <typename T> struct CornerAttribute : GenericAttribute<T> {
    CornerAttribute(Surface &m) : GenericAttribute<T>(m.ncorners())  {
        m.attr_corners.push_back(this->ptr);
    }
};

#endif //__ATTRIBUTES_H__

