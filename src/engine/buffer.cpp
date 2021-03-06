/* Created by Darren Otgaar on 2016/03/27. http://www.github.com/otgaard/zap */
#include "buffer.hpp"
#include "gl_api.hpp"

using namespace zap::engine;
using namespace zap::engine::gl;

buffer::~buffer() {
    if(is_allocated()) deallocate();
}

bool buffer::allocate() {
    gl::glGenBuffers(1, &id_);
    LOG("Buffer Allocated:", id_);
    return is_allocated() && !gl_error_check();
}

void buffer::deallocate() {
    if(is_mapped()) LOG_WARN("Buffer still mapped during deallocation");
    gl::glDeleteBuffers(1, &id_);
    LOG("Buffer Deallocated:", id_);
    id_ = INVALID_RESOURCE;
    size_ = 0;
}

void buffer::bind(buffer_type type) const {
    assert(is_allocated() && ZERR_UNALLOCATED_BUFFER);
    gl::glBindBuffer(gl::gl_type(type), id_);
    gl_error_check();
}

void buffer::release(buffer_type type) const {
    gl::glBindBuffer(gl::gl_type(type), 0);
}

bool buffer::is_bound() const {
    LOG_WARN("OpenGL query operation invoked");
    GLint bound = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
    return id_ == (resource_t)bound;
}

bool buffer::initialise(buffer_type type, buffer_usage usage, size_t size, const char* data) {
    assert(is_allocated() && ZERR_UNALLOCATED_BUFFER);
    //assert(is_bound() && "Attempt to initialise unbound buffer");
    glBufferData(gl_type(type), size, data, gl_type(usage));
    if(gl_error_check()) return false;
    size_ = size;
    return true;
}

bool buffer::orphan(buffer_type type, buffer_usage usage) {
    assert(is_allocated() && ZERR_UNALLOCATED_BUFFER);
    //assert(is_bound() && "Attempt to orphan unbound buffer");
    glBufferData(gl_type(type), size_, nullptr, gl_type(usage));
    return !gl_error_check();
}

bool buffer::copy(buffer_type type, size_t offset, size_t size, const char* data) {
    assert(is_allocated() && (offset + size) <= size_ && "Buffer unallocated or too small");
    glBufferSubData(gl_type(type), offset, size, data);
    return !gl_error_check();
}

const char* buffer::map(buffer_type type) const {
    assert(is_allocated() && ZERR_UNALLOCATED_BUFFER);
    mapped_ptr_ = reinterpret_cast<char*>(glMapBuffer(gl_type(type), gl_type(buffer_access::BA_READ_ONLY)));
    if(!mapped_ptr_) return nullptr;
    if(gl_error_check()) {
        glUnmapBuffer(gl_type(type));
        return nullptr;
    }
    return mapped_ptr_;
}

char* buffer::map(buffer_type type, buffer_access access) {
    assert(is_allocated() && ZERR_UNALLOCATED_BUFFER);
    mapped_ptr_ = reinterpret_cast<char*>(glMapBuffer(gl_type(type), gl_type(access)));
    if(!mapped_ptr_) return nullptr;
    if(gl_error_check()) {
        glUnmapBuffer(gl_type(type));
        return nullptr;
    }
    return mapped_ptr_;
}

GLbitfield gl_access(range_access::code access) {
    using namespace zap::maths;
    range_access::code tbl[log2_pow2<uint32_t>(range_access::BA_SIZE)] =
            {range_access::BA_MAP_READ, range_access::BA_MAP_WRITE, range_access::BA_MAP_PERSISTENT,
             range_access::BA_MAP_COHERENT, range_access::BA_MAP_INVALIDATE_RANGE, range_access::BA_MAP_INVALIDATE_BUFFER,
             range_access::BA_MAP_FLUSH_EXPLICIT, range_access::BA_MAP_UNSYNCHRONISED};
    GLbitfield map[log2_pow2<uint32_t>(range_access::BA_SIZE)] =
            {GL_MAP_READ_BIT, GL_MAP_WRITE_BIT, GL_MAP_PERSISTENT_BIT, GL_MAP_COHERENT_BIT, GL_MAP_INVALIDATE_RANGE_BIT,
             GL_MAP_INVALIDATE_BUFFER_BIT, GL_MAP_FLUSH_EXPLICIT_BIT, GL_MAP_UNSYNCHRONIZED_BIT};
    GLbitfield field = 0;
    for(int i = 0, end = log2_pow2<uint32_t>(range_access::BA_SIZE); i != end; ++i) {
        field |= (access & tbl[i] ? map[i] : 0);
    }
    return field;
}

char* buffer::map(buffer_type type, range_access::code access, size_t offset, size_t length) {
    assert(is_allocated() && (offset + length) <= size_ && "Buffer unallocated or too small");
    mapped_ptr_ = reinterpret_cast<char*>(glMapBufferRange(gl_type(type), offset, length, gl_access(access)));
    if(!mapped_ptr_) return nullptr;
    if(gl_error_check()) {
        unmap(type);
        return nullptr;
    }
    return mapped_ptr_;
}

void buffer::flush(buffer_type type, size_t offset, size_t length) {
    assert(is_allocated() && (offset + length) <= size_ && "Buffer unallocated or too small");
    glFlushMappedBufferRange(gl_type(type), offset, length);
    gl_error_check();
}

bool buffer::unmap(buffer_type type) const {
    assert(is_allocated() && is_mapped() && "Buffer unallocated or unmapped");
    glUnmapBuffer(gl_type(type));
    mapped_ptr_ = nullptr;
    return !gl_error_check();
}

bool buffer::copy_buffer(buffer_type src_type, buffer_type trg_type, size_t src_offset, size_t trg_offset, size_t length) {
    assert(is_allocated() && "Buffer unallocated or unmapped");
    glCopyBufferSubData(gl_type(src_type), gl_type(trg_type), src_offset, trg_offset, length);
    return !gl_error_check();
}