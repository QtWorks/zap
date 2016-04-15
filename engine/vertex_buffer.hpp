/* Created by Darren Otgaar on 2016/04/06. http://www.github.com/otgaard/zap */
#ifndef ZAP_VERTEX_BUFFER_HPP
#define ZAP_VERTEX_BUFFER_HPP

#include "buffer.hpp"
#include "buffer_format.hpp"

namespace zap { namespace engine {

namespace gl {
    void vertex_attrib_ptr(uint32_t index, int32_t size, data_type type, bool normalized, uint32_t stride, const void* ptr);
    void vertex_attrib_iptr(uint32_t index, int32_t size, data_type type, uint32_t stride, const void* ptr);
    void vertex_attrib_lptr(uint32_t index, int32_t size, data_type type, uint32_t stride, const void* ptr);
}

template <typename VTX_T, buffer_usage USAGE>
class vertex_buffer : public buffer {
public:
    static_assert(is_specialisation_of<vertex, VTX_T>::value, "VTX_T must be a specialisation of vertex<>");

    using vertex_t = VTX_T;
    using iterator = vertex_iterator<VTX_T>;
    using pod_t = typename vertex_t::pod_t;
    constexpr static auto buf_type = buffer_type::BT_ARRAY;
    constexpr static auto usage = USAGE;

    inline vertex_buffer() : vertex_count_(0) { }

    inline void bind() const { buffer::bind(buf_type); }
    inline void release() const { buffer::release(buf_type); }

    inline bool initialise(size_t vertex_count, const char* data=nullptr) {
        return buffer::initialise(buf_type, usage, vertex_count*vertex_t::size(), data) && configure_attributes();
    }

    inline bool initialise(const std::vector<char>& data) {
        return buffer::initialise(buf_type, usage, data) && configure_attributes();
    }

    inline bool initialise(const std::vector<vertex_t>& data) {
        auto result = buffer::initialise(buf_type, usage, vertex_t::size()*data.size(),
                                         reinterpret_cast<const char*>(data.data()));
        vertex_count_ = data.size();
        return result && configure_attributes();
    }

    // All sizes are in vertices, i.e src_off = 0 is the first vertex, src_off = 1 is the second and so on.
    bool copy(const vertex_buffer& src, size_t src_off, size_t trg_off, size_t vertex_count);

    inline char* map(buffer_access access) { return buffer::map(buf_type, access); }
    inline char* map(buffer_access access, size_t offset, size_t length) {
        return buffer::map(buf_type, access, offset, length);
    }
    inline bool unmap() { return buffer::unmap(buf_type); }

    inline iterator begin() {
        assert(is_mapped() && "Vertex Buffer must be mapped!");
        return iterator(reinterpret_cast<vertex_t*>(mapped_ptr_));
    }
    inline iterator end() {
        return iterator(reinterpret_cast<vertex_t*>(mapped_ptr_ + (size_ / vertex_t::size()) * vertex_t::size()));
    }
    inline const iterator begin() const {
        assert(is_mapped() && "Vertex Buffer must be mapped!");
        return iterator(reinterpret_cast<vertex_t*>(mapped_ptr_));
    }
    inline const iterator end() const {
        return iterator(reinterpret_cast<vertex_t*>(mapped_ptr_ + (size_ / vertex_t::size()) * vertex_t::size()));
    }

    inline vertex_t* data() {
        assert(is_mapped() && "Vertex Buffer must be mapped!");
        return reinterpret_cast<vertex_t*>(mapped_ptr_);
    }
    inline const vertex_t* data() const {
        assert(is_mapped() && "Vertex Buffer must be mapped!");
        return reinterpret_cast<vertex_t*>(mapped_ptr_);
    }

    inline size_t vertex_count() const { return vertex_count_; }

protected:
    bool configure_attributes();
    size_t vertex_count_;
};

template <typename VTX_T, buffer_usage USAGE>
bool vertex_buffer<VTX_T,USAGE>::configure_attributes() {
    static_assert(vertex_t::attrib_count() > 0, "An empty vertex<> type cannot be configured");

    if(vertex_t::attrib_count() == 1) {
        gl::vertex_attrib_ptr(maths::log2_pow2(vertex_t::types::data[0]), vertex_t::counts::data[0],
                              (data_type)vertex_t::datatypes::data[0], false, 0, nullptr);
        if(gl_error_check()) return false;
    } else {
        for(size_t i = 0; i != vertex_t::attrib_count(); ++i) {
            gl::vertex_attrib_ptr(maths::log2_pow2(vertex_t::types::data[i]), vertex_t::counts::data[i],
                                  (data_type)vertex_t::datatypes::data[i], false, vertex_t::size(),
                                  (void*)vertex_t::offsets::data[i]);
            if(gl_error_check()) return false;
        }
    }

    return true;
}

template <typename VTX_T, buffer_usage USAGE>
bool vertex_buffer<VTX_T,USAGE>::copy(const vertex_buffer& src, size_t src_off, size_t trg_off, size_t vertex_count) {
    const size_t length = vertex_count * VTX_T::size();
    const size_t source_offset = src_off * VTX_T::size();
    const size_t target_offset = trg_off * VTX_T::size();
    if(source_offset + length <= src.size_ && target_offset + length <= size_) {
        src.buffer::bind(buffer_type::BT_COPY_READ);
        buffer::bind(buffer_type::BT_COPY_WRITE);
        copy_buffer(buffer_type::BT_COPY_READ, buffer_type::BT_COPY_WRITE, source_offset, target_offset, length);
        buffer::release(buffer_type::BT_COPY_WRITE);
        src.buffer::release(buffer_type::BT_COPY_READ);
        return true;
    }
    return false;
}

}}

#endif //ZAP_VERTEX_BUFFER_HPP
