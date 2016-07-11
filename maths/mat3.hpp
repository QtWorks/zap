//
// Created by Darren Otgaar on 2016/03/08.
//

#ifndef ZAP_MAT3_HPP
#define ZAP_MAT3_HPP

#include "maths.hpp"
#include "vec2.hpp"
#include "vec3.hpp"
#include "mat2.hpp"

/* Note:
 * Matrices are represented in standard mathematical notation:
 * The matrix M has R rows and C columns.
 * An element is represented by M[r,c].
 * Matrices are stored in column major order.
 *
 * Note that memory in C++ is allocated in row major form, thus the initialiser is transposed
 *
 *  [ {{ 0, 1, 2 }, ] T   --> {{ 0, 3, 6 },  // Col 0
 *  [  { 3, 4, 5 }, ]          { 1, 4, 7 },  // Col 1
 *  [  { 6, 7, 8 }} ]          { 2, 5, 8 }}  // Col 2
 *  So that matrices may be post-multiplied.
 *
 *  Note that the mat3 structure is aligned to 3 __m128 registers and therefore wastes
 *  3 floats as a 3 x 4 matrix.
 */

namespace zap { namespace maths {
    template <typename T>
    struct ALIGN_DECL(16) mat3 {
        using type = T;

        static_assert(std::is_floating_point<T>::value || std::is_integral<T>::value, "mat3<T>: T must be an algebraic type");

        using row_t = vec3<T>;
        using col_t = vec3<T>;
        using vec_t = vec2<T>;
        using rot_t = mat2<T>;

        constexpr static size_t size() { return 9; }
        constexpr static size_t bytesize() { return sizeof(mat3<T>); }
        constexpr static size_t cols() { return 3; }
        constexpr static size_t rows() { return 3; }
        constexpr static size_t idx(size_t row, size_t col) { return col*(rows()+1) + row; }

        mat3() { }
        constexpr explicit mat3(T init) : m00(init), m10(init), m20(init),
                                          m01(init), m11(init), m21(init),
                                          m02(init), m12(init), m22(init) { }
        constexpr mat3(const static_list<T, 9>& lst) : m00(lst[0]), m10(lst[3]), m20(lst[6]),
                                                       m01(lst[1]), m11(lst[4]), m21(lst[7]),
                                                       m02(lst[2]), m12(lst[5]), m22(lst[8]) { }
        constexpr mat3(T m00, T m01, T m02, T m10, T m11, T m12, T m20, T m21, T m22) : m00(m00), m10(m10), m20(m20),
                                                                                        m01(m01), m11(m11), m21(m21),
                                                                                        m02(m02), m12(m12), m22(m22) { }
        constexpr mat3(T m00, T m11, T m22) : m00(m00),  m10(T(0)), m20(T(0)),
                                              m01(T(0)), m11(m11),  m21(T(0)),
                                              m02(T(0)), m12(T(0)), m22(m22) { }
        constexpr mat3(const col_t& col0, const col_t& col1, const col_t& col2) :
                m00(col0[0]), m10(col0[1]), m20(col0[2]),
                m01(col1[0]), m11(col1[1]), m21(col1[2]),
                m02(col2[0]), m12(col2[1]), m22(col2[2]) { }
        constexpr mat3(const mat3& rhs) : m00(rhs.m00), m10(rhs.m10), m20(rhs.m20),
                                          m01(rhs.m01), m11(rhs.m11), m21(rhs.m21),
                                          m02(rhs.m02), m12(rhs.m12), m22(rhs.m22) { }
        mat3& operator=(const mat3& rhs) {
            if(this != &rhs) {
                m00 = rhs.m00; m10 = rhs.m10; m20 = rhs.m20;
                m01 = rhs.m01; m11 = rhs.m11; m21 = rhs.m21;
                m02 = rhs.m02; m12 = rhs.m12; m22 = rhs.m22;
            }
            return *this;
        }

        constexpr static mat3 make_row(const row_t& row0, const row_t& row1, const row_t& row2) {
            return mat3(row0[0], row0[1], row0[2], row1[0], row1[1], row1[2], row2[0], row2[1], row2[2]);
        }
        constexpr static mat3 make_col(const col_t& col0, const col_t& col1, const col_t& col2) {
            return mat3(col0[0], col1[0], col2[0], col0[1], col1[1], col2[1], col0[2], col1[2], col2[2]);
        }

        constexpr static mat3 identity() {
            return mat3(T(1),T(1),T(1));
        }

        // Need to write a custom iterator for dealing with the 16 byte alignment

        const T& operator()(size_t row, size_t col) const {
            assert(row < rows() && col < cols() && ZERR_IDX_OUT_OF_RANGE);
            return arr[idx(row,col)];
        }

        T& operator()(size_t row, size_t col) {
            assert(row < rows() && col < cols() && ZERR_IDX_OUT_OF_RANGE);
            return arr[idx(row,col)];
        }

        vec3<T> col3(size_t col) const {
            checkidx(col, cols());
            return vec3<T>(operator()(0,col), operator()(1,col), operator()(2,col));
        }
        vec2<T> col2(size_t col) const {
            checkidx(col, cols());
            return vec2<T>(operator()(0,col), operator()(1,col));
        }
        void column(size_t col, const vec3<T>& v) {
            checkidx(col, cols());
            assert( ( ((col < 2) && eq(v.z, T(0))) || ((col == 2) && eq(v.z, T(1))) ) && "0..1 must be vectors, 2 must be a point");
            operator()(0,col) = v[0]; operator()(1,col) = v[1]; operator()(2,col) = v[2];
        }
        void column(size_t col, const vec2<T>& v) {
            checkidx(col, cols());
            operator()(0,col) = v[0]; operator()(1,col) = v[1];
        }

        mat3<T>& operator+=(const mat3& rhs) { for(const auto& i : { 0, 1, 2, 4, 5, 6, 8, 9, 10 }) (*this)[i] += rhs[i]; }
        mat3<T>& operator-=(const mat3& rhs) { for(const auto& i : { 0, 1, 2, 4, 5, 6, 8, 9, 10 }) (*this)[i] += rhs[i]; }


        union {
            struct {
                T m00, m10, m20, __m30,     // Column 0
                  m01, m11, m21, __m31,     // Column 1
                  m02, m12, m22, __m32;     // Column 2
            };
            T arr[cols()*(rows()+1)];
#ifdef ZAP_MATHS_SSE2
            __m128 xmm[cols()];
#endif //ZAP_MATHS_SSE2
        };
	} ALIGN_ATTR(16);

    using mat3b = mat3<uint8_t>;
    using mat3s = mat3<int16_t>;
    using mat3i = mat3<int32_t>;
    using mat3f = mat3<float>;
    using mat3d = mat3<double>;
}}

#endif //ZAP_MAT3_HPP
