#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "surface.h"
#include "attributes.h"
#include "mesh_io.h"

#include <zlib/zlib.h>

namespace UM {
	// supposes .obj file to have "f " entries without slashes
	// TODO: improve the parser
	// TODO: export vn and vt attributes
	void read_wavefront_obj(const std::string filename, Polygons &m) {
		m = Polygons();
		std::ifstream in;
		in.open (filename, std::ifstream::in);
		if (in.fail()) {
		    std::cerr << "Failed to open " << filename << std::endl;
		    return;
		}
		std::string line;
		while (!in.eof()) {
		    std::getline(in, line);
		    std::istringstream iss(line.c_str());
		    char trash;
		    if (!line.compare(0, 2, "v ")) {
		        iss >> trash;
		        vec3 v;
		        for (int i=0;i<3;i++) iss >> v[i];
		        m.points.data->push_back(v);
		    } else if (!line.compare(0, 2, "f ")) {
		        std::vector<int> f;
		        int idx;
		        iss >> trash;
		        while (iss >> idx) {
		            f.push_back(--idx);  // in wavefront obj all indices start at 1, not zero
		        }
		        int off_f = m.create_facets(1, f.size());
		        for (int i=0; i<static_cast<int>(f.size()); i++) {
		            m.vert(off_f, i) = f[i];
		        }
		    }
		}
		in.close();
		std::cerr << "#v: " << m.nverts() << " #f: "  << m.nfacets() << std::endl;
	}

	void write_wavefront_obj(const std::string filename, const Surface &m) {
		std::fstream out;
		out.open(filename, std::ios_base::out);
		out << std::fixed << std::setprecision(4);
		for (int v=0; v<m.nverts(); v++)
		    out << "v " << m.points[v] << std::endl;
		for (int f=0; f<m.nfacets(); f++) {
		    out << "f ";
		    for (int v=0; v<m.facet_size(f); v++)
		        out << (m.vert(f,v)+1) << " ";
		    out << std::endl;
		}
		out.close();
	}

	typedef unsigned int index_t;
	/*
	// Attention: only int and double attributes
	void write_geogram_ascii(const std::string filename, const Surface &m, SurfaceAttributes attr) {
		auto [pattr, fattr, cattr] = attr;
		std::fstream file;
		file.open(filename, std::ios_base::out);
		file << "[HEAD]\n\"GEOGRAM\"\n\"1.0\"\n[ATTS]\n\"GEO::Mesh::vertices\"\n";
		file << m.nverts() << "\n";
		file << "[ATTR]\n\"GEO::Mesh::vertices\"\n\"point\"\n\"double\"\n8 # this is the size of an element (in bytes)\n3 # this is the number of elements per item\n";
		file << std::fixed << std::setprecision(4);
		for (const vec3 &p : m.points)
		    file << p.x << "\n" << p.y << "\n" << p.z << "\n";
		file << "[ATTS]\n\"GEO::Mesh::facets\"\n";
		file << m.nfacets() << "\n";
		file << "[ATTR]\n\"GEO::Mesh::facets\"\n\"GEO::Mesh::facets::facet_ptr\"\n\"index_t\"\n4\n1\n";
		for (int f=0; f<m.nfacets(); f++)
		    file << m.facet_corner(f,0) << "\n";

		file << "[ATTS]\n\"GEO::Mesh::facet_corners\"\n";
		file << m.ncorners() << "\n";
		file << "[ATTR]\n\"GEO::Mesh::facet_corners\"\n\"GEO::Mesh::facet_corners::corner_vertex\"\n\"index_t\"\n4\n1\n";
		for (int f=0; f<m.nfacets(); f++)
		    for (int v=0; v<m.facet_size(f); v++)
		       file << m.vert(f,v) << "\n";
		file << "[ATTR]\n\"GEO::Mesh::facet_corners\"\n\"GEO::Mesh::facet_corners::corner_adjacent_facet\"\n\"index_t\"\n4\n1\n";
		SurfaceConnectivity fec(m);
		for (int c=0; c<m.ncorners(); c++) {
		    int opp = fec.opposite(c);
		    file << (opp < 0 ? index_t(-1) : fec.c2f[opp]) << "\n";
		}

		// TODO ugly, repair it
		std::vector<NamedContainer> A[3] = {std::get<0>(attr), std::get<1>(attr), std::get<2>(attr)};
		for (int i=0; i<3; i++) {
		    auto att = A[i];

		    for (int i=0; i<static_cast<int>(att.size()); i++) {
		        std::string name = att[i].first;
		        std::shared_ptr<GenericAttributeContainer> ptr = att[i].second;

		        if (i==0)
		            file << "[ATTR]\n\"GEO::Mesh::vertices\"\n\"" << name << "\"\n";
		        else if (i==1)
		            file << "[ATTR]\n\"GEO::Mesh::facets\"\n\"" << name << "\"\n";
		        else
		            file << "[ATTR]\n\"GEO::Mesh::facet_corners\"\n\"" << name << "\"\n";

		        if (auto aint = std::dynamic_pointer_cast<AttributeContainer<int> >(ptr); aint.get()!=nullptr) {
		            file << "\"int\"\n4\n1\n";
		            for (const auto &v : aint->data)
		                file << v << "\n";
		        } else if (auto adouble = std::dynamic_pointer_cast<AttributeContainer<double> >(ptr); adouble.get()!=nullptr) {
		            file << "\"double\"\n8\n1\n";
		            for (const auto &v : adouble->data)
		                file << v << "\n";
		        } else {
		            assert(false);
		        }
		    }
		}

		file.close();
	}
	*/

    namespace {
        struct GeogramGZWriter {
            GeogramGZWriter(std::string const &fName) : file_() {
                file_ = gzopen(fName.c_str(), "wb");
                if (!file_)
                    throw std::runtime_error("Can not open file");
            }

            ~GeogramGZWriter() {
                if (file_)
                    gzclose(file_);
            }

            void addFileHeader() {
                addHeader("HEAD");
                addU64(4+7+4+3);
                addString("GEOGRAM");
                addString("1.0");
            }

            void addAttributeSize(std::string const &wh, size_t n) {
                addHeader("ATTS");
                addU64(4+wh.length()+4);
                addString(wh);
                addU32(uint32_t(n));
            }

            template <typename T> void addAttribute(std::string const &wh, std::string const &name, std::string const &type, std::vector<T> const &data, const int dim=1) {
                addHeader("ATTR");
                addU64((4+wh.length())+(4+name.length())+(4+type.length())+4+4+sizeof(T)*data.size());
                addString(wh);
                addString(name);
                addString(type);
                addU32(sizeof(T));
                addU32(dim);
                addData(static_cast<void const *>(data.data()), sizeof(T)*data.size());
            }

            void addAttribute(std::string const &wh, std::string const &name, std::vector<vec2> const &data) {
                addHeader("ATTR");
                addU64((4+wh.length())+(4+name.length())+(4+6)+4+4+sizeof(double)*2*data.size());
                addString(wh);
                addString(name);
                addString("double");
                addU32(8);
                addU32(2);
                std::vector<double> values;
                for (const auto &p : data) {
                    values.push_back(p.x);
                    values.push_back(p.y);
                }
                addData(static_cast<void const *>(values.data()), sizeof(double)*values.size());
            }

            void addAttribute(std::string const &wh, std::string const &name, std::vector<vec3> const &data) {
                addHeader("ATTR");
                addU64((4+wh.length())+(4+name.length())+(4+6)+4+4+sizeof(double)*3*data.size());
                addString(wh);
                addString(name);
                addString("double");
                addU32(8);
                addU32(3);
                std::vector<double> values;
                for (const auto &p : data) {
                    values.push_back(p.x);
                    values.push_back(p.y);
                    values.push_back(p.z);
                }
                addData(static_cast<void const *>(values.data()), sizeof(double)*values.size());
            }

            protected:
            void addU64(uint64_t size) {
                addData(&size, 8);
            }

            void addU32(uint32_t index) {
                addData(&index, 4);
            }

            void addHeader(std::string const &header) {
                if (header.length()!=4)
                    throw std::runtime_error("GeogramGZWriter::bad header");
                addData(static_cast<void const *>(header.c_str()), 4);
            }

            void addString(std::string const &str) {
                size_t len = str.length();
                addU32(uint32_t(len));
                addData(str.c_str(),len);
            }

            void addData(void const *data, size_t len) {
                int check = gzwrite(file_, data, (unsigned int)(len));
                if (size_t(check) != len)
                    throw std::runtime_error("Could not write attribute data");
            }

            gzFile file_;
        };
    }

    void write_geogram(const std::string filename, const PolyLine &pl, PolyLineAttributes attr) {
        try {
            GeogramGZWriter writer(filename);
            writer.addFileHeader();
            writer.addAttributeSize("GEO::Mesh::vertices", pl.nverts());
            writer.addAttribute("GEO::Mesh::vertices", "point", *pl.points.data);

            writer.addAttributeSize("GEO::Mesh::edges", pl.nsegments());
            {
                std::vector<index_t> segments;
                for (int s : pl.segments) segments.push_back(s);
                writer.addAttribute("GEO::Mesh::edges", "GEO::Mesh::edges::edge_vertex", "index_t", segments, 2);
            }

            std::vector<NamedContainer> A[2] = {std::get<0>(attr), std::get<1>(attr)};
            for (int z=0; z<2; z++) {
                auto &att = A[z];

                for (int i=0; i<static_cast<int>(att.size()); i++) {
                    std::string name = att[i].first;
                    std::shared_ptr<GenericAttributeContainer> ptr = att[i].second;
                    std::string place = "";

                    if (z==0)
                        place = "GEO::Mesh::vertices";
                    else if (z==1)
                        place = "GEO::Mesh::edges";

                    std::cerr << place << " " << name << std::endl;
                    if (auto aint = std::dynamic_pointer_cast<AttributeContainer<int> >(ptr); aint.get()!=nullptr) {
                        writer.addAttribute(place, name, "int", aint->data);
                    } else if (auto adouble = std::dynamic_pointer_cast<AttributeContainer<double> >(ptr); adouble.get()!=nullptr) {
                        writer.addAttribute(place, name, "double", adouble->data);
                    } else if (auto avec2 = std::dynamic_pointer_cast<AttributeContainer<vec2> >(ptr); avec2.get()!=nullptr) {
                        writer.addAttribute(place, name, avec2->data);
                    } else if (auto avec3 = std::dynamic_pointer_cast<AttributeContainer<vec3> >(ptr); avec3.get()!=nullptr) {
                        writer.addAttribute(place, name, avec3->data);
                    } else {
                        assert(false);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Ooops: catch error= " << e.what() << " when creating " << filename << "\n";
        }
    }

    void write_geogram(const std::string filename, const Volume &m, VolumeAttributes attr) {
        try {
            GeogramGZWriter writer(filename);

            if (!m.nverts()) return;

            writer.addFileHeader();
            writer.addAttributeSize("GEO::Mesh::vertices", m.nverts());
            writer.addAttribute("GEO::Mesh::vertices", "point", *m.points.data);

            if (!m.ncells()) return;

            writer.addAttributeSize("GEO::Mesh::cells", m.ncells());

            std::vector<char> cell_type(m.ncells());
            for (int c=0; c<m.ncells(); c++) cell_type[c] = m.cell_type();
            writer.addAttribute("GEO::Mesh::cells", "GEO::Mesh::cells::cell_type", "char", cell_type);

            std::vector<index_t> cell_ptr(m.ncells());
            for (int c=1; c<m.ncells(); c++) cell_ptr[c] = cell_ptr[c-1]+m.nverts_per_cell();
            writer.addAttribute("GEO::Mesh::cells", "GEO::Mesh::cells::cell_ptr", "index_t", cell_ptr);

            writer.addAttributeSize("GEO::Mesh::cell_corners", m.ncorners());
            std::vector<index_t> corner_vertex;
            for (int c=0; c<m.ncells(); c++)
                for (int v=0; v<m.nverts_per_cell(); v++)
                    corner_vertex.push_back(m.vert(c,v));
            writer.addAttribute("GEO::Mesh::cell_corners", "GEO::Mesh::cell_corners::corner_vertex", "index_t", corner_vertex);

            // TODO GEO::Mesh::cell_facets::adjacent_cell

            std::vector<NamedContainer> A[4] = {std::get<0>(attr), std::get<1>(attr), std::get<2>(attr), std::get<3>(attr)};
            for (int z=0; z<4; z++) {
                auto &att = A[z];

                for (int i=0; i<static_cast<int>(att.size()); i++) {
                    std::string name = att[i].first;
                    std::shared_ptr<GenericAttributeContainer> ptr = att[i].second;
                    std::string place = "";

                    if (z==0)
                        place = "GEO::Mesh::vertices";
                    else if (z==1)
                        place = "GEO::Mesh::cells";
                    else if (z==2)
                        place = "GEO::Mesh::cell_facets";
                    else
                        place = "GEO::Mesh::cell_corners";

                    if (auto aint = std::dynamic_pointer_cast<AttributeContainer<int> >(ptr); aint.get()!=nullptr) {
                        writer.addAttribute(place, name, "int", aint->data);
                    } else if (auto adouble = std::dynamic_pointer_cast<AttributeContainer<double> >(ptr); adouble.get()!=nullptr) {
                        writer.addAttribute(place, name, "double", adouble->data);
                    } else if (auto avec2 = std::dynamic_pointer_cast<AttributeContainer<vec2> >(ptr); avec2.get()!=nullptr) {
                        writer.addAttribute(place, name, avec2->data);
                    } else if (auto avec3 = std::dynamic_pointer_cast<AttributeContainer<vec3> >(ptr); avec3.get()!=nullptr) {
                        writer.addAttribute(place, name, avec3->data);
                    } else {
                        assert(false);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Ooops: catch error= " << e.what() << " when creating " << filename << "\n";
        }
    }

    void write_geogram(const std::string filename, const Surface &m, SurfaceAttributes attr) {
        try {
            GeogramGZWriter writer(filename);
            writer.addFileHeader();
            writer.addAttributeSize("GEO::Mesh::vertices", m.nverts());
            writer.addAttribute("GEO::Mesh::vertices", "point", *m.points.data);

            writer.addAttributeSize("GEO::Mesh::facets", m.nfacets());
            std::vector<index_t> facet_ptr;
            for (int f=0; f<m.nfacets(); f++) facet_ptr.push_back(m.facet_corner(f,0));
            writer.addAttribute("GEO::Mesh::facets", "GEO::Mesh::facets::facet_ptr", "index_t", facet_ptr);

            writer.addAttributeSize("GEO::Mesh::facet_corners", m.ncorners());
            std::vector<index_t> corner_vertex;
            for (int f=0; f<m.nfacets(); f++)
                for (int v=0; v<m.facet_size(f); v++)
                    corner_vertex.push_back(m.vert(f,v));
            writer.addAttribute("GEO::Mesh::facet_corners", "GEO::Mesh::facet_corners::corner_vertex", "index_t", corner_vertex);

            std::vector<index_t> corner_adjacent_facet;
            SurfaceConnectivity fec(m);
            for (int c=0; c<m.ncorners(); c++) {
                int opp = fec.opposite(c);
                corner_adjacent_facet.push_back(opp < 0 ? index_t(-1) : fec.c2f[opp]);
            }
            writer.addAttribute("GEO::Mesh::facet_corners", "GEO::Mesh::facet_corners::corner_adjacent_facet", "index_t", corner_adjacent_facet);

            std::vector<NamedContainer> A[3] = {std::get<0>(attr), std::get<1>(attr), std::get<2>(attr)};
            for (int z=0; z<3; z++) {
                auto &att = A[z];

                for (int i=0; i<static_cast<int>(att.size()); i++) {
                    std::string name = att[i].first;
                    std::shared_ptr<GenericAttributeContainer> ptr = att[i].second;
                    std::string place = "";

                    if (z==0)
                        place = "GEO::Mesh::vertices";
                    else if (z==1)
                        place = "GEO::Mesh::facets";
                    else
                        place = "GEO::Mesh::facet_corners";

                    if (auto aint = std::dynamic_pointer_cast<AttributeContainer<int> >(ptr); aint.get()!=nullptr) {
                        writer.addAttribute(place, name, "int", aint->data);
                    } else if (auto adouble = std::dynamic_pointer_cast<AttributeContainer<double> >(ptr); adouble.get()!=nullptr) {
                        writer.addAttribute(place, name, "double", adouble->data);
                    } else if (auto avec2 = std::dynamic_pointer_cast<AttributeContainer<vec2> >(ptr); avec2.get()!=nullptr) {
                        writer.addAttribute(place, name, avec2->data);
                    } else if (auto avec3 = std::dynamic_pointer_cast<AttributeContainer<vec3> >(ptr); avec3.get()!=nullptr) {
                        writer.addAttribute(place, name, avec3->data);
                    } else {
                        //      std::cerr << place << std::endl;
                        //        TODO : ignore adjacency
                        assert(false);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Ooops: catch error= " << e.what() << " when creating " << filename << "\n";
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct GeogramGZReader {
        GeogramGZReader(std::string const &filename) :
            file_(nullptr),
            current_chunk_class_("0000"),
            current_chunk_size_(0),
            current_chunk_file_pos_(0) {

                //        std::cerr << "GeogramGZReader()\n";
                file_ = gzopen(filename.c_str(), "rb");
                if (!file_)
                    throw std::runtime_error("Can not open file");

                //        std::cerr << "read_chunk_header()\n";
                read_chunk_header();
                if (current_chunk_class_ != "HEAD") {
                    throw std::runtime_error(filename + " Does not start with HEAD chunk");
                }

                //        std::cerr << "read_string()\n";
                std::string magic = read_string();
                if(magic != "GEOGRAM") {
                    throw std::runtime_error(filename + " is not a GEOGRAM file");
                }
                std::string version = read_string();
                //          Logger::out("I/O") << "GeoFile version: " << version << std::endl;
                check_chunk_size();
            }

        ~GeogramGZReader() {
            if (file_)
                gzclose(file_);
        }

        index_t read_int() {
            std::uint32_t result = 0;
            int check = gzread(file_, &result, sizeof(std::uint32_t));
            if (!check && gzeof(file_))
                result = std::uint32_t(-1);
            else if(size_t(check) != sizeof(std::uint32_t))
                throw std::runtime_error("Could not read integer from file");
            return result;
        }

        std::string read_string() {
            std::string result;
            index_t len = read_int();
            result.resize(len);
            if (len) {
                int check = gzread(file_, &result[0], (unsigned int)(len));
                if (index_t(check) != len)
                    throw std::runtime_error("Could not read string data from file");
            }
            return result;
        }

        void check_chunk_size() {
            long chunk_size = gztell(file_) - current_chunk_file_pos_;
            if (current_chunk_size_ != chunk_size)
                throw std::runtime_error(std::string("Chunk size mismatch: ") + " expected " + std::to_string(current_chunk_size_) + "/ got " + std::to_string(chunk_size));
        }

        std::string read_chunk_class() {
            std::string result;
            result.resize(4,'\0');
            int check = gzread(file_, &result[0], 4);
            if (check == 0 && gzeof(file_))
                result = "EOFL";
            else if (check != 4)
                throw std::runtime_error("Could not read chunk class from file");
            return result;
        }

        size_t read_size() {
            std::uint64_t result = 0;
            int check = gzread(file_, &result, sizeof(std::uint64_t));
            if (check == 0 && gzeof(file_))
                result = size_t(-1);
            else
                if (size_t(check) != sizeof(std::uint64_t))
                    throw std::runtime_error("Could not read size from file");
            return size_t(result);
        }

        void read_chunk_header() {
            current_chunk_class_ = read_chunk_class();
            if (gzeof(file_)) {
                gzclose(file_);
                file_ = nullptr;
                current_chunk_size_ = 0;
                current_chunk_class_ = "EOFL";
                return;
            }
            current_chunk_size_ = long(read_size());
            current_chunk_file_pos_ = gztell(file_);
        }

        void skip_chunk() {
            gzseek(file_, current_chunk_size_ + current_chunk_file_pos_, SEEK_SET);
        }

        const std::string& next_chunk() {
            // If the file pointer did not advance as expected
            // between two consecutive calls of next_chunk, it
            // means that the client code does not want to
            // read the current chunk, then it needs to be
            // skipped.
            if (gztell(file_) != current_chunk_file_pos_ + current_chunk_size_)
                skip_chunk();

            read_chunk_header();
            return current_chunk_class_;
        }

        void read_attribute(void* addr, size_t size) {
            assert(current_chunk_class_ == "ATTR");
            int check = gzread(file_, addr, (unsigned int)(size));
            if (size_t(check) != size)
                throw std::runtime_error("Could not read attribute  (" + std::to_string(check) + "/" + std::to_string(size) + " bytes read)");
            check_chunk_size();
        }

        gzFile file_;
        std::string current_chunk_class_;
        long current_chunk_size_;
        long current_chunk_file_pos_;
    };

/*
	void read_geogram(const std::string filename, PointSet &pointset, PolyLine &polyline, Polygons &polygons, std::vector<NamedContainer> &pt_attr, std::vector<NamedContainer> &edg_attr, std::vector<NamedContainer> &fct_attr, std::vector<NamedContainer> &crn_attr, std::vector<NamedContainer> &cell_attr, std::vector<NamedContainer> &cell_fct_attr, std::vector<NamedContainer> &cell_crn_attr) {
		try {
		    GeogramGZReader in(filename);
		    std::string chunk_class;
		    for (chunk_class=in.next_chunk(); chunk_class!="EOFL"; chunk_class=in.next_chunk()) {
		        if (chunk_class == "ATTS") {
		            std::string attribute_set_name = in.read_string();
		            index_t nb_items = in.read_int();
		            in.check_chunk_size();
		            std::cerr << "ATTS " << attribute_set_name << " " << nb_items << std::endl;

		            if (attribute_set_name == "GEO::Mesh::vertices") {
		                pointset.resize(nb_items);
		            } else if (attribute_set_name == "GEO::Mesh::facets") {
		                polygons.offset.resize(nb_items+1);
		                for (index_t o=0; o<nb_items+1; o++) polygons.offset[o] = 3*o;
		            } else if (attribute_set_name == "GEO::Mesh::facet_corners") {
		                polygons.facets.resize(nb_items);
		            } else if (attribute_set_name == "GEO::Mesh::edges") {
		                polyline.segments.resize(nb_items*2);
		            }
		        } else if (chunk_class == "ATTR") {
		            std::string attribute_set_name = in.read_string();
		            int nb_items = 0;
		            if (attribute_set_name == "GEO::Mesh::vertices") {
		                nb_items = pointset.size();
		            } else if (attribute_set_name == "GEO::Mesh::facets") {
		                nb_items = polygons.offset.size()-1;
		            } else if (attribute_set_name == "GEO::Mesh::facet_corners") {
		                nb_items = polygons.facets.size();
		            } else if (attribute_set_name == "GEO::Mesh::edges") {
		                nb_items = polyline.nsegments();
		            } else {
		                continue;
		            }
		            assert(nb_items>0);

		            std::string attribute_name = in.read_string();
		            std::string element_type = in.read_string();
		            index_t element_size = in.read_int();
		            index_t dimension = in.read_int();
		            size_t size = size_t(element_size) * size_t(dimension) * size_t(nb_items);

		            std::cerr << "ATTR " << attribute_set_name << " " << attribute_name << " " << element_type << " " << element_size << " " << dimension << "\n";

		            if (attribute_set_name == "GEO::Mesh::vertices" && attribute_name == "point") {
		                assert(element_size==8);
		                std::vector<double> raw(nb_items*dimension);
		                in.read_attribute((void *)raw.data(), size);
		                for (int v=0; v<nb_items; v++)
		                    pointset[v] = {raw[v*3+0], raw[v*3+1], raw[v*3+2]};
		            } else if (attribute_set_name == "GEO::Mesh::facets" && attribute_name == "GEO::Mesh::facets::facet_ptr") {
		                in.read_attribute((void *)polygons.offset.data(), size);
		            } else if (attribute_set_name == "GEO::Mesh::facet_corners" && attribute_name == "GEO::Mesh::facet_corners::corner_vertex") {
		                in.read_attribute((void *)polygons.facets.data(), size);
	//                  std::cerr << "size " << size <<std::endl;
		            } else if (attribute_set_name == "GEO::Mesh::edges" && attribute_name == "GEO::Mesh::edges::edge_vertex") {
		                in.read_attribute((void *)polyline.segments.data(), size);
	//                  std::cerr << "size " << size <<std::endl;
		            } else if (attribute_name!="GEO::Mesh::facet_corners::corner_adjacent_facet") {
		                std::shared_ptr<GenericAttributeContainer> P;
		                if (element_type=="int" || element_type=="index_t" || element_type=="signed_index_t") {
		                    GenericAttribute<int> A(nb_items);
		                    void *ptr = std::dynamic_pointer_cast<AttributeContainer<int> >(A.ptr)->data.data();
		                    in.read_attribute(ptr, size);
		                    P = A.ptr;
		                } else if (element_type=="double") {
		                    assert(element_size==8);
		                    std::vector<double> raw(nb_items*dimension);
		                    in.read_attribute((void *)raw.data(), size);

		                    if (1==dimension) {
		                        GenericAttribute<double> A(nb_items);
		                        for (int i=0; i<nb_items; i++)
		                            A[i] = raw[i];
		                        P = A.ptr;
		                    } if (2==dimension) {
		                        GenericAttribute<vec2> A(nb_items);
		                        for (int i=0; i<nb_items; i++)
		                                A[i] = {raw[i*2+0], raw[i*2+1]};
		                         P = A.ptr;
		                   } if (3==dimension) {
		                        GenericAttribute<vec3> A(nb_items);
		                        for (int i=0; i<nb_items; i++)
		                                A[i] = {raw[i*3+0], raw[i*3+1], raw[i*3+2]};
		                         P = A.ptr;
		                   }
		                }

		                if (attribute_set_name == "GEO::Mesh::vertices") {
		                    pt_attr.emplace_back(attribute_name, P);
		                    pointset.attr.emplace_back(P);
		                } else if (attribute_set_name == "GEO::Mesh::facets") {
		                    fct_attr.emplace_back(attribute_name, P);
		                    polygons.attr_facets.emplace_back(P);
		                } else if (attribute_set_name == "GEO::Mesh::facet_corners") {
		                    crn_attr.emplace_back(attribute_name, P);
		                    polygons.attr_corners.emplace_back(P);
		                } else if (attribute_set_name == "GEO::Mesh::edges") {
		                    edg_attr.emplace_back(attribute_name, P);
		                    polyline.attr.emplace_back(P);
		                }
		            }
		        } // chunk_class = ATTR
		    } // chunks
		    polygons.offset.back() = polygons.facets.size();
		} catch (const std::exception& e) {
		    std::cerr << "Ooops: catch error= " << e.what() << " when reading " << filename << "\n";
		}

	//  for (auto &a :  pt_attr) pointset.attr.emplace_back(a.second);
	//  for (auto &a : fct_attr) polygons.attr_facets.emplace_back(a.second);
	//  for (auto &a : crn_attr) polygons.attr_corners.emplace_back(a.second);
	}
*/

    const std::string attrib_set_names[7] = {"GEO::Mesh::vertices", "GEO::Mesh::edges", "GEO::Mesh::facets", "GEO::Mesh::facet_corners", "GEO::Mesh::cells", "GEO::Mesh::cell_facets", "GEO::Mesh::cell_corners"};
    void read_geogram(const std::string filename, std::vector<NamedContainer> attr[7]) {
        int set_size[7] = {-1, -1, -1, -1, -1, -1, -1};
        try {
            GeogramGZReader in(filename);
            std::string chunk_class;
            for (chunk_class=in.next_chunk(); chunk_class!="EOFL"; chunk_class=in.next_chunk()) {
                if (chunk_class == "ATTS") {
                    std::string attribute_set_name = in.read_string();
                    index_t nb_items = in.read_int();
                    in.check_chunk_size();
                    std::cerr << "ATTS " << attribute_set_name << " " << nb_items << std::endl;
                    for (int i=0; i<7; i++)
                        if (attribute_set_name == attrib_set_names[i])
                            set_size[i] = nb_items;
                } else if (chunk_class == "ATTR") {
                    std::string attribute_set_name = in.read_string();
                    int nb_items = -1;
                    for (int i=0; i<7; i++)
                        if (attribute_set_name == attrib_set_names[i])
                            nb_items = set_size[i];

                    assert(nb_items>0);

                    std::string attribute_name = in.read_string();
                    std::string element_type   = in.read_string();
                    index_t element_size = in.read_int();
                    index_t dimension    = in.read_int();
                    size_t size = size_t(element_size) * size_t(dimension) * size_t(nb_items);

                    if (attribute_name=="GEO::Mesh::cell_facets::adjacent_cell" || attribute_name=="GEO::Mesh::facet_corners::corner_adjacent_facet") continue;

                    std::cerr << "ATTR " << attribute_set_name << " " << attribute_name << " " << element_type << " " << element_size << " " << dimension << "\n";

                    std::shared_ptr<GenericAttributeContainer> P;
                    if (element_type=="char") {
                        assert(dimension == 1);
                        std::vector<char> tmp(nb_items);
                        in.read_attribute(tmp.data(), size);
                        GenericAttribute<int> A(nb_items);
                        for (int i=0; i<nb_items; i++) {
                            A[i] = tmp[i];
                        }
                        P = A.ptr;
                    } else if (element_type=="int" || element_type=="index_t" || element_type=="signed_index_t") {
                        GenericAttribute<int> A(nb_items);
                        if (attribute_name=="GEO::Mesh::edges::edge_vertex")
                            A = GenericAttribute<int>(nb_items*2); // TODO AARGH Bruno!
                        assert(dimension == 1 || (attribute_name=="GEO::Mesh::edges::edge_vertex" && dimension == 2));
                        void *ptr = std::dynamic_pointer_cast<AttributeContainer<int> >(A.ptr)->data.data();
                        in.read_attribute(ptr, size);
                        P = A.ptr;
                    } else if (element_type=="double") {
                        assert(element_size==8);
                        std::vector<double> raw(nb_items*dimension);
                        in.read_attribute((void *)raw.data(), size);

                        if (1==dimension) {
                            GenericAttribute<double> A(nb_items);
                            for (int i=0; i<nb_items; i++)
                                A[i] = raw[i];
                            P = A.ptr;
                        } if (2==dimension) {
                            GenericAttribute<vec2> A(nb_items);
                            for (int i=0; i<nb_items; i++)
                                A[i] = {raw[i*2+0], raw[i*2+1]};
                            P = A.ptr;
                        } if (3==dimension) {
                            GenericAttribute<vec3> A(nb_items);
                            for (int i=0; i<nb_items; i++)
                                A[i] = {raw[i*3+0], raw[i*3+1], raw[i*3+2]};
                            P = A.ptr;
                        }
                    } else if (element_type=="vec2") {
                        assert(element_size==16 || dimension==1);
                        std::vector<double> raw(nb_items*dimension);
                        GenericAttribute<vec2> A(nb_items);
                        void *ptr = std::dynamic_pointer_cast<AttributeContainer<vec3> >(A.ptr)->data.data();
                        in.read_attribute(ptr, size);
                        P = A.ptr;
                    } else if (element_type=="vec3") {
                        assert(element_size==24 || dimension==1);
                        std::vector<double> raw(nb_items*dimension);
                        GenericAttribute<vec3> A(nb_items);
                        void *ptr = std::dynamic_pointer_cast<AttributeContainer<vec3> >(A.ptr)->data.data();
                        in.read_attribute(ptr, size);
                        P = A.ptr;
                    } else {
                        // TODO bool
                        continue;
                    }

                    for (int i=0; i<7; i++)
                        if (attribute_set_name == attrib_set_names[i])
                            attr[i].emplace_back(attribute_name, P);
                } // chunk_class = ATTR
            } // chunks
        } catch (const std::exception& e) {
            std::cerr << "Ooops: catch error= " << e.what() << " when reading " << filename << "\n";
        }
    }

    void parse_pointset_attributes(PointSet &pts, std::vector<NamedContainer> &attr) {
        for (int i=0; i<(int)attr.size(); i++) {
            if (attr[i].first != "point") continue;
            std::shared_ptr<AttributeContainer<vec3> > ptr = std::dynamic_pointer_cast<AttributeContainer<vec3> >(attr[i].second);
            pts.resize(ptr->data.size());
            for (int v=0; v<pts.size(); v++)
                pts[v] = ptr->data[v];
            attr.erase(attr.begin()+i);
            i--;
        }
    }

    void parse_corner_vertex(std::vector<int> &corner_vertex, std::vector<NamedContainer> &attr) {
        for (int i=0; i<(int)attr.size(); i++) {
            if (attr[i].first != "GEO::Mesh::cell_corners::corner_vertex") continue;
            std::shared_ptr<AttributeContainer<int> > ptr = std::dynamic_pointer_cast<AttributeContainer<int> >(attr[i].second);
            corner_vertex = ptr->data;
            attr.erase(attr.begin()+i);
            i--;
        }
    }

    void parse_cell_type_cell_ptr(std::vector<int> &cell_type, std::vector<int> &cell_ptr, std::vector<NamedContainer> &attr) {
        for (int i=0; i<(int)attr.size(); i++) {
            if (attr[i].first != "GEO::Mesh::cells::cell_type" && attr[i].first != "GEO::Mesh::cells::cell_ptr") continue;
            std::vector<int> &arr =  (attr[i].first == "GEO::Mesh::cells::cell_type" ? cell_type : cell_ptr);
            std::shared_ptr<AttributeContainer<int> > ptr = std::dynamic_pointer_cast<AttributeContainer<int> >(attr[i].second);
            arr = ptr->data;
            attr.erase(attr.begin()+i);
            i--;
        }
    }

    const int  nb_verts_per_cell_type[5] = {4, 8, 6, 5, 4}; // geogram types: tet, hex, prism, pyramid, connector
    const int nb_facets_per_cell_type[5] = {4, 6, 5, 5, 3};

    void parse_volume_data(const std::string filename, PointSet &pts, VolumeAttributes &va, std::vector<int> &corner_vertex, int type2keep) {
        std::vector<NamedContainer> attrib[7];
        read_geogram(filename, attrib);
        parse_pointset_attributes(pts, attrib[0]);

        std::vector<int> old_corner_vertex;
        parse_corner_vertex(old_corner_vertex, attrib[6]);
        std::vector<int> cell_type(old_corner_vertex.size()/4, 0); // if cell_type and cell_ptr are not present in .geogram, create it (tet mesh)
        std::vector<int> cell_ptr(old_corner_vertex.size()/4, 0);
        for (int i=0; i<(int)cell_type.size(); i++) cell_ptr[i] = i*4;
        parse_cell_type_cell_ptr(cell_type, cell_ptr, attrib[4]);

        int ncells = cell_type.size();
        assert(cell_ptr.size()==cell_type.size());
        cell_ptr.push_back(old_corner_vertex.size());

        corner_vertex = std::vector<int>();
        corner_vertex.reserve(old_corner_vertex.size());

        std::vector<int> cells_old2new(ncells,  -1);
        std::vector<int> corners_old2new(old_corner_vertex.size(), -1);
        int nfacets = 0;
        for (int c=0; c<ncells; c++) {
            assert(cell_type[c]>=0 && cell_type[c]<=6);
            nfacets += nb_facets_per_cell_type[cell_type[c]];
        }
        std::vector<int> facets_old2new(nfacets, -1);

        int new_ncells = 0;
        int new_ncorners = 0;
        int new_nfacets = 0;
        for (int c=0; c<ncells; c++) {
            if (type2keep!=cell_type[c]) continue;
            for (int v=cell_ptr[c]; v<cell_ptr[c+1]; v++)
                corner_vertex.push_back(old_corner_vertex[v]);

            cells_old2new[c] = new_ncells++;

            for (int i=0; i<nb_verts_per_cell_type[cell_type[c]]; i++)
                corners_old2new[new_ncorners] = new_ncorners+i;
            new_ncorners += nb_verts_per_cell_type[cell_type[c]];

            for (int i=0; i<nb_facets_per_cell_type[cell_type[c]]; i++)
                facets_old2new[new_nfacets] = new_nfacets+i;
            new_nfacets += nb_facets_per_cell_type[cell_type[c]];
        }

        for (auto &nc : attrib[4])
            (*nc.second).compress(cells_old2new);
        for (auto &nc : attrib[5])
            (*nc.second).compress(facets_old2new);
        for (auto &nc : attrib[6])
            (*nc.second).compress(corners_old2new);

        std::get<0>(va) = attrib[0];
        std::get<1>(va) = attrib[4];
        std::get<2>(va) = attrib[5];
        std::get<3>(va) = attrib[6];
    }

    VolumeAttributes read_geogram(const std::string filename, Tetrahedra &m) {
        VolumeAttributes va;
        std::vector<int> corner_vertex;
        parse_volume_data(filename, m.points, va, corner_vertex, 0);
        assert(corner_vertex.size()%4==0);

        int nhexa = corner_vertex.size()*4;
        m.create_tets(nhexa);
        m.cells = corner_vertex;

        return va;
    }

    VolumeAttributes read_geogram(const std::string filename, Hexahedra &m) {
        VolumeAttributes va;
        std::vector<int> corner_vertex;
        parse_volume_data(filename, m.points, va, corner_vertex, 1);
        assert(corner_vertex.size()%8==0);

        int nhexa = corner_vertex.size()*8;
        m.create_hexa(nhexa);
        m.cells = corner_vertex;

        return va;
    }

	SurfaceAttributes read_geogram(const std::string filename, Polygons &polygons) {
		polygons = Polygons();
		PolyLine polyline;
		std::vector<NamedContainer> pt_attr, edg_attr, fct_attr, crn_attr, cell_attr, cell_fct_attr, cell_crn_attr;
////	read_geogram(filename, polygons.points, polyline, polygons, pt_attr, edg_attr, fct_attr, crn_attr);
		return make_tuple(pt_attr, fct_attr, crn_attr);
	}

	PolyLineAttributes read_geogram(const std::string filename, PolyLine &pl) {
		Polygons polygons;
		pl = PolyLine();
		std::vector<NamedContainer> pt_attr, edg_attr, fct_attr, crn_attr;
////	read_geogram(filename, pl.points, pl, polygons, pt_attr, edg_attr, fct_attr, crn_attr);
		return make_tuple(pt_attr, edg_attr);
	}

}

