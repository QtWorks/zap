/* Created by Darren Otgaar on 2016/04/16. http://www.github.com/otgaard/zap */
#ifndef ZAP_MESH_HPP
#define ZAP_MESH_HPP

#include "engine.hpp"
#include "index_buffer.hpp"
#include "vertex_buffer.hpp"

/*
 * TODO: These classes need serious work.  The intention is to remain strongly-typed wherever possible.
 */

namespace zap { namespace engine {

class ZAPENGINE_EXPORT mesh_base {
public:
    mesh_base(size_t vertex_count, data_type idx_type=data_type::DT_VOID, size_t index_count=0)
            : idx_type_(idx_type), vertex_count_(vertex_count), index_count_(index_count) {
    }
    mesh_base(const mesh_base&) = delete;
    mesh_base(mesh_base&& rhs) noexcept : vao_(rhs.vao_), idx_type_(rhs.idx_type_),
                                          vertex_count_(rhs.vertex_count_), index_count_(rhs.index_count_) {
        rhs.vao_ = INVALID_RESOURCE;
    }
    virtual ~mesh_base() { if(is_allocated()) deallocate(); vao_ = INVALID_RESOURCE; }

    mesh_base& operator=(mesh_base&& rhs) noexcept {
        if(this != &rhs) {
            std::swap(vao_, rhs.vao_);
            idx_type_ = rhs.idx_type_;
            vertex_count_ = rhs.vertex_count_;
            index_count_ = rhs.index_count_;
            rhs.vao_ = INVALID_IDX;
        }
        return *this;
    }
    mesh_base& operator=(const mesh_base&& rhs) = delete;

    bool is_allocated() const { return vao_ != INVALID_RESOURCE; }
    bool is_indexed() const { return idx_type_ != data_type::DT_VOID; }

    data_type get_index_type() const { return idx_type_; }
    void set_vertex_count(size_t count) { vertex_count_ = count; }
    size_t get_vertex_count() const { return index_count_; }
    void set_index_count(size_t count) { index_count_ = count; }
    size_t get_index_count() const { return index_count_; }

    bool allocate();
    void deallocate();

    void bind() const;
    void release() const;

    void set_attrib_divisor(uint32_t va_idx, uint32_t divisor);
    void draw_arrays_impl(primitive_type type, uint32_t first, uint32_t count) const;
    void draw_arrays_inst_impl(primitive_type type, uint32_t first, uint32_t count, uint32_t instances) const;
    void draw_arrays_inst_baseinst_impl(primitive_type type, uint32_t first, uint32_t count, uint32_t instances, uint32_t offset) const;
    void draw_elements_impl(primitive_type type, data_type index_type, uint32_t first, uint32_t count) const;
    void draw_elements_inst_impl(primitive_type type, data_type index_type, uint32_t first, uint32_t count, uint32_t instances) const;
    void draw_elements_inst_baseinst_impl(primitive_type type, data_type index_type, uint32_t first, uint32_t count, uint32_t instances, uint32_t offset) const;

    void draw(primitive_type type) const {
        is_indexed() ? draw_elements_impl(type, idx_type_, 0, uint32_t(index_count_)) : draw_arrays_impl(type, 0, uint32_t(vertex_count_));
    }

private:
    resource_t vao_ = INVALID_RESOURCE;
    data_type idx_type_;
    size_t vertex_count_;
    size_t index_count_;
};

template <typename... VBuffers>
struct vertex_stream {
    virtual void free() { }
    virtual void bind() { }
    virtual void append(buffer** arr) { }
};

template <typename VBuf, typename... VBuffers>
struct vertex_stream<VBuf, VBuffers...> : vertex_stream<VBuffers...> {
    static_assert(is_vertex_buffer<VBuf>::value, "VBuf must be of type vertex_buffer<>");
    using type = VBuf;
    using ptr_t = VBuf*;
    static const size_t count = sizeof...(VBuffers) + 1;
    vertex_stream() : ptr(nullptr) { }
    vertex_stream(ptr_t vbuf_ptr, VBuffers*... vbuffers)
            : vertex_stream<VBuffers...>(vbuffers...), ptr(vbuf_ptr) { }
    virtual ~vertex_stream() = default;
    virtual void free() {
        vertex_stream<VBuffers...>::free();
        delete ptr;
    }

    virtual void append(buffer** arr) {
        *arr++ = ptr;
        vertex_stream<VBuffers...>::append(arr);
    }

    ptr_t ptr;
};

template <size_t k, typename... VBuffers> struct stream_query;

template <typename VBuf, typename... VBuffers>
struct stream_query<0, vertex_stream<VBuf, VBuffers...>> {
    using type = VBuf;
};

template <size_t k, typename VBuf, typename... VBuffers>
struct stream_query<k, vertex_stream<VBuf, VBuffers...>> {
    using type = typename stream_query<k-1, vertex_stream<VBuffers...>>::type;
};

template <size_t k, typename... VBuffers>
constexpr typename std::enable_if<k==0, typename stream_query<0, vertex_stream<VBuffers...>>::type>::type* get(vertex_stream<VBuffers...>& D) {
    return D.ptr;
};

template <size_t k, typename VBuf, typename... VBuffers>
constexpr typename std::enable_if<k!=0, typename stream_query<k, vertex_stream<VBuf, VBuffers...>>::type>::type* get(vertex_stream<VBuf, VBuffers...>& D) {
    vertex_stream<VBuffers...>& base = D;
    return get<k-1>(base);
};

template <typename VtxStream, typename Index=void>
struct mesh : public mesh_base {
public:
    static_assert(is_index_buffer<Index>::value, "Index must be of type index_buffer<>");
    using vertex_stream_t = VtxStream;
    using index_buffer_t = Index;
    constexpr static data_type idx_t = (data_type)dt_descriptor<typename Index::type>::value;

    mesh() : mesh_base(0, idx_t, 0), idx_buffer_ptr(nullptr) { }
    explicit mesh(const mesh&) = delete;
    mesh(const vertex_stream_t& vtxstream, index_buffer_t* idx_ptr, bool own_vstream=false, bool own_ibuffer=false) :
            mesh_base(0, idx_t, 0),
            vstream(vtxstream),
            idx_buffer_ptr(idx_ptr),
            own_vstream(own_vstream),
            own_ibuffer(own_ibuffer) {
    }
    mesh(mesh&& rhs) noexcept :
            mesh_base(std::move(rhs)),
            vstream(rhs.vstream),
            idx_buffer_ptr(rhs.idx_buffer_ptr),
            own_vstream(rhs.own_vstream),
            own_ibuffer(rhs.own_ibuffer) {
        rhs.own_vstream = false;
        rhs.own_ibuffer = false;
    }
    ~mesh() override {
        if(own_vstream) vstream.free();
        if(own_ibuffer) delete idx_buffer_ptr;
    }

    mesh& operator=(const mesh&) = delete;
    mesh& operator=(mesh&& rhs) noexcept {
        if(this != &rhs) {
            mesh_base::operator=(std::move(rhs));
            if(own_vstream) vstream.free();
            vstream = rhs.vstream;
            own_vstream = rhs.own_vstream;
            rhs.own_vstream = false;
            if(own_ibuffer) delete idx_buffer_ptr;
            idx_buffer_ptr = rhs.idx_buffer_ptr;
            own_ibuffer = rhs.own_ibuffer;
            rhs.own_ibuffer = false;
        }
        return *this;
    }

    void set_stream(const vertex_stream_t& vtxstream, bool own=false) {
        vstream = vtxstream;
        set_vertex_count(vstream.ptr->vertex_count());
        own_vstream = own;
    }
    void set_index(index_buffer_t* idx_ptr, bool own=false) {
        idx_buffer_ptr = idx_ptr;
        set_index_count(idx_buffer_ptr->index_count());
        own_ibuffer = own;
    }
    size_t vertex_count() const { return vstream.ptr ? vstream.ptr->vertex_count() : 0; }
    size_t index_count() const { return idx_buffer_ptr->index_count(); }
    // This interface is really broken
    void update_counts() {
        set_vertex_count(vstream.ptr->vertex_count());
        set_index_count(idx_buffer_ptr->index_count());
    }

    void draw(primitive_type type, size_t start=0, size_t count=0) {
        if(!idx_buffer_ptr) { LOG_ERR("No index specified"); return; }

        mesh_base::draw_elements_impl(type,
                                      (data_type)dt_descriptor<typename index_buffer_t::type>::value,
                                      uint32_t(start),
                                      uint32_t(count == 0 ? idx_buffer_ptr->index_count() : count));
    }

    void draw_inst(primitive_type type, size_t instances, size_t start=0, size_t count=0) {
        if(!idx_buffer_ptr) { LOG_ERR("No index specified"); return; }

        mesh_base::draw_elements_inst_impl(type,
                                           (data_type)dt_descriptor<typename index_buffer_t::type>::value,
                                           uint32_t(start),
                                           uint32_t(count == 0 ? idx_buffer_ptr->index_count() : count),
                                           uint32_t(instances));
    }

    vertex_stream_t vstream;
    Index* idx_buffer_ptr = nullptr;
    bool own_vstream = false;
    bool own_ibuffer = false;
};

template <typename VtxStream>
struct mesh<VtxStream, void> : public mesh_base {
public:
    using vertex_stream_t = VtxStream;

    mesh() : mesh_base(0) { }
    explicit mesh(const vertex_stream_t& vtxstream, bool own=false) :
            mesh_base(0),
            vstream(vtxstream),
            own_vstream(own) {
    }
    mesh(const mesh&) = delete;
    mesh(mesh&& rhs) noexcept :
            mesh_base(std::move(rhs)),
            vstream(rhs.vstream),
            own_vstream(rhs.own_vstream) {
        rhs.own_vstream = false;
    }
    ~mesh() override {
        if(own_vstream) vstream.free();
    }

    mesh& operator=(const mesh&) = delete;
    mesh& operator=(mesh&& rhs) noexcept {
        if(this != &rhs) {
            mesh_base::operator=(std::move(rhs));
            if(own_vstream) vstream.free();
            vstream = rhs.vstream;
            own_vstream = rhs.own_vstream;
            rhs.own_vstream = false;
        }
        return *this;
    }

    void set_stream(const vertex_stream_t& vtxstream, bool own=false) {
        vstream = vtxstream;
        set_vertex_count(vstream.ptr->vertex_count());
        own_vstream = own;
    }
    size_t vertex_count() const { return vstream.ptr ? vstream.ptr->vertex_count() : 0; }
    void update_counts() { set_vertex_count(vstream.ptr->vertex_count()); }

    void draw(primitive_type type, size_t start=0, size_t count=0, bool no_null_stream=true) {
        if(no_null_stream && !vstream.ptr) { LOG("No vertex stream specified"); return; }
        mesh_base::draw_arrays_impl(type, uint32_t(start), uint32_t(count == 0 ?vstream.ptr->vertex_count() : count));
    }

    vertex_stream_t vstream;
    bool own_vstream = false;
};

template <typename VertexT>
mesh<vertex_stream<vertex_buffer<VertexT>>> make_mesh(size_t vertex_count, buffer_usage usage=buffer_usage::BU_STATIC_DRAW) {
#if defined(__APPLE__) || defined(_WIN32)
    auto m = mesh<vertex_stream<vertex_buffer<VertexT>>>();
#else
    auto m = std::unique_ptr<mesh<vertex_stream<vertex_buffer<VertexT>>>>(new mesh<vertex_stream<vertex_buffer<VertexT>>>{});
#endif
    auto vbuf_ptr = std::make_unique<vertex_buffer<VertexT>>();
    vbuf_ptr->usage(usage);

    if(m.allocate() && vbuf_ptr->allocate()) {
        m.bind(); vbuf_ptr->bind();
        if(vbuf_ptr->initialise(vertex_count)) {
            m.set_stream(vertex_stream<vertex_buffer<VertexT>>{vbuf_ptr.release()}, true);
            m.release();
            gl_error_check();
            return m;
        } else {
            LOG_ERR("Failed to initialise vertex_buffer");
        }
    } else {
        LOG_ERR("Failed to allocate mesh resources");
    }

    m.release();
    return m;
}

template <typename VertexT>
std::unique_ptr<mesh<vertex_stream<vertex_buffer<VertexT>>>> make_mesh(const std::vector<VertexT>& buffer, buffer_usage usage=buffer_usage::BU_STATIC_DRAW) {
#if defined(__APPLE__) || defined(_WIN32)
    auto m = std::make_unique<mesh<vertex_stream<vertex_buffer<VertexT>>>>();
#else
    auto m = std::unique_ptr<mesh<vertex_stream<vertex_buffer<VertexT>>>>(new mesh<vertex_stream<vertex_buffer<VertexT>>>{});
#endif
    auto vbuf_ptr = std::make_unique<vertex_buffer<VertexT>>();

    vbuf_ptr->usage(usage);
    if(m->allocate() && vbuf_ptr->allocate()) {
        m->bind(); vbuf_ptr->bind();
        if(vbuf_ptr->initialise(buffer)) {
            m->set_stream(vertex_stream<vertex_buffer<VertexT>>{vbuf_ptr.release()}, true);
            m->release();
            gl_error_check();
            return m;
        } else {
            LOG_ERR("Failed to initialise vertex_buffer");
        }
    } else {
        LOG_ERR("Failed to allocate mesh resources");
    }

    m.release();
    return m;
}

template <typename VertexT, typename IndexT>
std::unique_ptr<mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>>> make_mesh(const std::tuple<std::vector<VertexT>, std::vector<IndexT>>& def) {
#if defined(__APPLE__) || defined(_WIN32)
    auto m = std::make_unique<mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>>>();
#else
    auto m = std::unique_ptr<mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>>>(new mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>>{});
#endif
    auto vbuf_ptr = std::make_unique<vertex_buffer<VertexT>>();
    auto ibuf_ptr = std::make_unique<index_buffer<IndexT>>();

    if(m->allocate() && vbuf_ptr->allocate() && ibuf_ptr->allocate()) {
        m->bind(); vbuf_ptr->bind(); ibuf_ptr->bind();
        if(vbuf_ptr->initialise(get<0>(def)) && ibuf_ptr->initialise(get<1>(def))) {
            m->set_stream(vertex_stream<vertex_buffer<VertexT>>{vbuf_ptr.release()}, true);
            m->set_index(ibuf_ptr.release(), true);
            m->release();
            gl_error_check();
            return m;
        } else {
            LOG_ERR("Failed to initialise vertex_buffer or index_buffer");
        }
    } else {
        LOG_ERR("Failed to allocate mesh resources");
    }

    m.release();
    return m;
}

template <typename VertexT, typename IndexT>
mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>>
make_mesh(size_t vertex_count, size_t index_count, buffer_usage usage=buffer_usage::BU_STATIC_DRAW) {
    mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>> m;

    auto vbuf_ptr = std::make_unique<vertex_buffer<VertexT>>();
    auto ibuf_ptr = std::make_unique<index_buffer<IndexT>>();

    vbuf_ptr->usage(usage);
    ibuf_ptr->usage(usage);

    if(m.allocate() && vbuf_ptr->allocate() && ibuf_ptr->allocate()) {
        m.bind(); vbuf_ptr->bind(); ibuf_ptr->bind();
        if(vbuf_ptr->initialise(vertex_count) && ibuf_ptr->initialise(index_count)) {
            m.set_stream(vertex_stream<vertex_buffer<VertexT>>{vbuf_ptr.release()}, true);
            m.set_index(ibuf_ptr.release(), true);
            m.release();
            gl_error_check();
            return m;
        } else {
            LOG_ERR("Failed to initialise vertex_buffer or index_buffer");
        }
    } else {
        LOG_ERR("Failed to allocate mesh resources");
    }

    m.release();
    m.deallocate();
    return m;
}

template <typename VertexT, typename IndexT>
mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>>
make_mesh(const std::pair<std::vector<VertexT>, std::vector<IndexT>>& obj, buffer_usage usage=buffer_usage::BU_STATIC_DRAW) {
    mesh<vertex_stream<vertex_buffer<VertexT>>, index_buffer<IndexT>> m;

    auto vbuf_ptr = std::make_unique<vertex_buffer<VertexT>>();
    auto ibuf_ptr = std::make_unique<index_buffer<IndexT>>();

    vbuf_ptr->usage(usage);
    ibuf_ptr->usage(usage);

    if(m.allocate() && vbuf_ptr->allocate() && ibuf_ptr->allocate()) {
        m.bind(); vbuf_ptr->bind(); ibuf_ptr->bind();
        if(vbuf_ptr->initialise(obj.first) && ibuf_ptr->initialise(obj.second)) {
            m.set_stream(vertex_stream<vertex_buffer<VertexT>>{vbuf_ptr.release()}, true);
            m.set_index(ibuf_ptr.release(), true);
            m.release();
            gl_error_check();
            return m;
        } else {
            LOG_ERR("Failed to initialise vertex_buffer or index_buffer");
        }
    } else {
        LOG_ERR("Failed to allocate mesh resources");
    }

    m.release();
    m.deallocate();
    return m;
}

template <typename Vertex0T, typename Vertex1T, typename IndexT>
mesh<vertex_stream<vertex_buffer<Vertex0T>, vertex_buffer<Vertex1T>>, index_buffer<IndexT>>
make_mesh(size_t v0_count, size_t v1_count, size_t index_count, buffer_usage usage=buffer_usage::BU_STATIC_DRAW) {
    using stream_t = vertex_stream<vertex_buffer<Vertex0T>, vertex_buffer<Vertex1T>>;
    mesh<stream_t, index_buffer<IndexT>> m;

    auto vbuf0_ptr = std::make_unique<typename stream_query<0, stream_t>::type>();
    auto vbuf1_ptr = std::make_unique<typename stream_query<1, stream_t>::type>();
    auto ibuf_ptr = std::make_unique<index_buffer<IndexT>>();

    vbuf0_ptr->usage(usage);
    vbuf1_ptr->usage(usage);
    ibuf_ptr->usage(usage);

    if(m.allocate() && vbuf0_ptr->allocate() && vbuf1_ptr->allocate() && ibuf_ptr->allocate()) {
        m.bind();
        vbuf0_ptr->bind();
        if (!vbuf0_ptr->initialise(v0_count)) {
            LOG_ERR("Failed to initialise vertex buffer 0");
            gl_error_check();
            return m;
        }

        vbuf1_ptr->bind();
        if (!vbuf1_ptr->initialise(v1_count)) {
            LOG_ERR("Failed to initialise vertex buffer 1");
            gl_error_check();
            return m;
        }

        ibuf_ptr->bind();
        if (!ibuf_ptr->initialise(index_count)) {
            LOG_ERR("Failed to initialise index buffer");
            gl_error_check();
            return m;
        }

        m.set_stream(stream_t{vbuf0_ptr.release(), vbuf1_ptr.release()}, true);
        m.set_index(ibuf_ptr.release(), true);
        m.release();
        gl_error_check();
        return m;
    }

    m.release();
    m.deallocate();
    return m;
}


}}

#endif //ZAP_MESH_HPP
