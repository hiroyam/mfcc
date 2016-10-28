#include <chrono>
#include <cstdio>
#include <cstdarg>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>


#include <immintrin.h>
#include <omp.h>

namespace cc {
/********************************************************************************
 *
 * config
 *
 ********************************************************************************/
using float_t = float;

/********************************************************************************
 *
 * sleep
 *
 ********************************************************************************/
inline void sleep_ms(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

/********************************************************************************
 *
 * format_str
 *
 ********************************************************************************/
inline std::string format_str(const char *fmt, ...) {
    static char buf[2048];
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
#ifdef _MSC_VER
#pragma warning(default:4996)
#endif
    return std::string(buf);
}

/********************************************************************************
 *
 * colorant
 *
 * @example
 * std::cout << colorant('g', "[OK]") << std::endl;
 *
 ********************************************************************************/
inline std::string colorant(const char color, const std::string str) {
    std::string ret;
    switch (color) {
        case 'r': ret = "\033[31m";
            break;
        case 'g': ret = "\033[32m";
            break;
        case 'y': ret = "\033[33m";
            break;
        case 'b': ret = "\033[34m";
            break;
        case 'm': ret = "\033[35m";
            break;
        case 'c': ret = "\033[36m";
            break;
        default:  ret = "\033[39m";
            break;
    }
    return ret + str + "\033[39m";
}

/********************************************************************************
 *
 * timer
 *
 ********************************************************************************/
class timer {
public:
    timer() : t1(std::chrono::high_resolution_clock::now()) {};
    double elapsed() {return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - t1).count(); }
    void   restart() {t1 = std::chrono::high_resolution_clock::now(); }
    void   start()   {t1 = std::chrono::high_resolution_clock::now(); }
    void   stop()    {t2 = std::chrono::high_resolution_clock::now(); }
    double total()   {stop(); return std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1).count(); }

    void print(const char *name = "") {
        std::cout << format_str("%-46s: %.3f sec", name, elapsed()) << std::endl;
    }

    void print_with_gflops(const char *name = "", double ops = 0.0) {
        std::cout << format_str("%-46s: %.3f sec : %.1f Gflops", name, elapsed(), ops / elapsed() / 1e9) << std::endl;
    }

    ~timer() {}

private:
    std::chrono::high_resolution_clock::time_point t1, t2;
};

/********************************************************************************
 *
 * progress_display
 *
 ********************************************************************************/
inline void progress_display(int numerator, int denominator) {
    const int bar_width = 50;
    int       progress  = 100 * numerator / (denominator - 1);
    progress = std::min(progress, 100);
    progress = std::max(progress, 0);

    int        pos      = bar_width * progress / 100;
    static int last_pos = 0;
    if (pos == last_pos) {
        return;
    }
    last_pos = pos;

    std::cout << "\r[";
    for (int i = 0; i < bar_width; i++) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << progress << "%" << std::flush;
    if (progress == 100) {
        std::cout << "\n" << std::flush;
    }
}

/********************************************************************************
 *
 * aligned_allocator
 *
 ********************************************************************************/
template<typename T, std::size_t alignment>
class aligned_allocator {
public:
    typedef T              value_type;
    typedef T             *pointer;
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T             &reference;
    typedef const T       &const_reference;
    typedef const T       *const_pointer;

    template<typename U>
    struct rebind {
        typedef aligned_allocator<U, alignment> other;
    };

    aligned_allocator() {}

    template<typename U>
    aligned_allocator(const aligned_allocator<U, alignment> &) {}

    const_pointer address(const_reference value) const {
        return std::addressof(value);
    }

    pointer address(reference value) const {
        return std::addressof(value);
    }

    pointer allocate(size_type size, const void * = nullptr) {
        void *p = aligned_alloc(alignment, sizeof(T) * size);
        if (!p && size > 0) throw std::runtime_error("failed to allocate");
        return static_cast<pointer>(p);
    }

    size_type max_size() const {
        return ~static_cast<std::size_t>(0) / sizeof(T);
    }

    void deallocate(pointer ptr, size_type) {
        aligned_free(ptr);
    }

    template<class U, class V>
    void construct(U *ptr, const V &value) {
        void *p = ptr;
        ::new(p)U(value);
    }

#if defined(_MSC_VER) && _MSC_VER <= 1800
    // -vc2013 doesn't support variadic templates
#else
    template<class U, class ... Args>
    void construct(U *ptr, Args && ... args) {
        void *p = ptr;
        ::new(p)U(std::forward<Args>(args) ...);
    }

#endif  // if defined(_MSC_VER) && _MSC_VER <= 1800
    template<class U>
    void construct(U *ptr) {
        void *p = ptr;
        ::new(p)U();
    }

    template<class U>
    void destroy(U *ptr) {
        ptr->~U();
    }

private:
    void *aligned_alloc(size_type align, size_type size) const {
#if defined(_MSC_VER)
        return ::_aligned_malloc(size, align);
#elif defined(__ANDROID__)
        return ::memalign(align, size);
#elif defined(__MINGW32__)
        return _mm_malloc(size, align);
#else       // posix assumed
        void *p;
        if (::posix_memalign(&p, align, size) != 0) {
            p = 0;
        }
        return p;
#endif      // if defined(_MSC_VER)
    }

    void aligned_free(pointer ptr) {
#if defined(_MSC_VER)
        ::_aligned_free(ptr);
#elif defined(__MINGW32__)
        ::free(ptr);
#else
        ::free(ptr);
#endif      // if defined(_MSC_VER)
    }
};

template<typename T1, typename T2, std::size_t alignment>
inline bool operator==(const aligned_allocator<T1, alignment> &, const aligned_allocator<T2, alignment> &) {
    return true;
}

template<typename T1, typename T2, std::size_t alignment>
inline bool operator!=(const aligned_allocator<T1, alignment> &, const aligned_allocator<T2, alignment> &) {
    return false;
}

using vec_t = std::vector<float_t, aligned_allocator<float_t, 64>>;

/********************************************************************************
 *
 * random
 *
 ********************************************************************************/
template<typename T>
inline
typename std::enable_if<std::is_integral<T>::value, T>::type uniform_rand(T min, T max) {
    // avoid gen(0) for MSVC known issue
    // https://connect.microsoft.com/VisualStudio/feedback/details/776456
    static std::mt19937              gen(1);
    std::uniform_int_distribution<T> dst(min, max);
    return dst(gen);
}

template<typename T>
inline
typename std::enable_if<std::is_floating_point<T>::value, T>::type uniform_rand(T min, T max) {
    static std::mt19937               gen(1);
    std::uniform_real_distribution<T> dst(min, max);
    return dst(gen);
}

template<typename T>
inline
typename std::enable_if<std::is_floating_point<T>::value, T>::type gaussian_rand(T mean, T sigma) {
    static std::mt19937         gen(1);
    std::normal_distribution<T> dst(mean, sigma);
    return dst(gen);
}

template<typename Container>
inline int uniform_idx(const Container &t) {
    return uniform_rand(0, int(t.size() - 1));
}

inline bool bernoulli(float_t p) {
    return uniform_rand(float_t(0), float_t(1)) <= p;
}

template<typename Iter>
void uniform_rand(Iter begin, Iter end, float_t min, float_t max) {
    for (Iter it = begin; it != end; ++it) *it = uniform_rand(min, max);
}

template<typename Iter>
void gaussian_rand(Iter begin, Iter end, float_t mean, float_t sigma) {
    for (Iter it = begin; it != end; ++it) *it = gaussian_rand(mean, sigma);
}

/********************************************************************************
 *
 * misc
 *
 ********************************************************************************/
template<typename T>
T *reverse_endian(T *p) {
    std::reverse(reinterpret_cast<char *>(p), reinterpret_cast<char *>(p) + sizeof(T));
    return p;
}

inline bool is_little_endian() {
    int x = 1;
    return *(char *)&x != 0;
}

template<typename T>
size_t max_index(const T &vec) {
    auto begin_iterator = std::begin(vec);
    return std::max_element(begin_iterator, std::end(vec)) - begin_iterator;
}

template<typename T, typename U>
U rescale(T x, T src_min, T src_max, U dst_min, U dst_max) {
    U value =  static_cast<U>(((x - src_min) * (dst_max - dst_min)) / (src_max - src_min) + dst_min);
    return std::min(dst_max, std::max(value, dst_min));
}

inline void nop() {
    // do nothing
}

/********************************************************************************
 *
 * is_near
 *
 ********************************************************************************/
inline bool is_near(float_t a, float_t b) {
    const float_t EPSILON = 1e-3;
    if (std::abs(a - b) <= EPSILON * std::max(std::abs(a), std::abs(b))) {
        return true;
    } else {
        return false;
    }
}

inline bool is_near(const vec_t &a, const vec_t &b) {
    if (a.size() != b.size()) {
        throw std::runtime_error("failed to compare vectors: vector size invalid");
    }
    for (size_t i = 0; i < a.size(); i++) {
//         if (std::abs(a[i] - b[i]) > 0.01f) {
//             static int count = 0;
//             if (count++ < 1080) {
//                 printf("DEBUG CODE: %d %.3f %.3f\n", i,  a[i], b[i]);
//             }
//         }
        if (!is_near(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

/********************************************************************************
 *
 * index3d
 *
 ********************************************************************************/
template<typename T>
struct index3d {
    index3d(T width, T height, T depth) {
        reshape(width, height, depth);
    }

    index3d() : width_(0), height_(0), depth_(0) {}

    void reshape(T width, T height, T depth) {
        width_  = width;
        height_ = height;
        depth_  = depth;

        if ((long long)width * height * depth > std::numeric_limits<T>::max())
            throw std::runtime_error(
                      format_str("error while constructing layer: layer size too large for tiny-cnn\nWidthxHeightxChannels=%dx%dx%d >= max size of [%s](=%d)",
                                 width, height, depth, typeid(T).name(), std::numeric_limits<T>::max()));
    }

    T get_index(T x, T y, T channel) const {
        assert(x >= 0 && x < width_);
        assert(y >= 0 && y < height_);
        assert(channel >= 0 && channel < depth_);
        return (height_ * channel + y) * width_ + x;
    }

    T area() const {
        return width_ * height_;
    }

    T size() const {
        return width_ * height_ * depth_;
    }

    T width_;
    T height_;
    T depth_;
};

typedef index3d<size_t> layer_shape_t;

template<typename Stream, typename T>
Stream &operator<<(Stream &s, const index3d<T> &d) {
    s << d.width_ << "x" << d.height_ << "x" << d.depth_;
    return s;
}

/********************************************************************************
 *
 * visualize
 *
 ********************************************************************************/
template<typename T = unsigned char>
class image {
public:
    typedef T intensity_t;

    image() : width_(0), height_(0), depth_(1) {}

    image(const T *data, size_t width, size_t height) : width_(width), height_(height), depth_(1), data_(depth_ * width_ * height_, 0) {
        memcpy(&data_[0], data, depth_ * width * height * sizeof(T));
    }

    image(index3d<size_t> rhs) : width_(rhs.width_), height_(rhs.height_), depth_(rhs.depth_), data_(depth_ * width_ * height_, 0) {}

    image(size_t width, size_t height) : width_(width), height_(height), depth_(1), data_(width * height, 0) {}

    image(const image &rhs) : width_(rhs.width_), height_(rhs.height_), depth_(rhs.depth_), data_(rhs.data_) {}

    image(const image &&rhs) : width_(rhs.width_), height_(rhs.height_), depth_(rhs.depth_), data_(std::move(rhs.data_)) {}

    image &operator=(const image &rhs) {
        width_  = rhs.width_;
        height_ = rhs.height_;
        depth_  = rhs.depth_;
        data_   = rhs.data_;
        return *this;
    }

    image &operator=(const image &&rhs) {
        width_  = rhs.width_;
        height_ = rhs.height_;
        depth_  = rhs.depth_;
        data_   = std::move(rhs.data_);
        return *this;
    }

    void write(const std::string &path) const {
        // WARNING: This is OS dependent (writes of bytes with reinterpret_cast depend on endianness)
        std::ofstream ofs(path.c_str(), std::ios::binary | std::ios::out);

        if (!is_little_endian()) throw std::runtime_error("image::write for bit-endian is not supported");

        const uint32_t line_pitch  = ((width_ + 3) / 4) * 4;
        const uint32_t header_size = 14 + 12 + 256 * 3;
        const uint32_t data_size   = line_pitch * height_;

        // file header(14 byte)
        const uint16_t file_type    = ('M' << 8) | 'B';
        const uint32_t file_size    = header_size + data_size;
        const uint32_t reserved     = 0;
        const uint32_t offset_bytes = header_size;

        ofs.write(reinterpret_cast<const char *>(&file_type), 2);
        ofs.write(reinterpret_cast<const char *>(&file_size), 4);
        ofs.write(reinterpret_cast<const char *>(&reserved), 4);
        ofs.write(reinterpret_cast<const char *>(&offset_bytes), 4);

        // info header(12byte)
        const uint32_t info_header_size = 12;
        const int16_t  width            = static_cast<int16_t>(width_);
        const int16_t  height           = static_cast<int16_t>(height_);
        const uint16_t planes           = 1;
        const uint16_t bit_count        = 8;

        ofs.write(reinterpret_cast<const char *>(&info_header_size), 4);
        ofs.write(reinterpret_cast<const char *>(&width), 2);
        ofs.write(reinterpret_cast<const char *>(&height), 2);
        ofs.write(reinterpret_cast<const char *>(&planes), 2);
        ofs.write(reinterpret_cast<const char *>(&bit_count), 2);

        // color palette (256*3byte)
        for (int i = 0; i < 256; i++) {
            const auto v = static_cast<const char>(i);
            ofs.write(&v, 1); // R
            ofs.write(&v, 1); // G
            ofs.write(&v, 1); // B
        }

        // data
        for (size_t i = 0; i < height_; i++) {
            ofs.write(reinterpret_cast<const char *>(&data_[(height_ - 1 - i) * width_]), width_);
            if (line_pitch != width_) {
                uint32_t dummy = 0;
                ofs.write(reinterpret_cast<const char *>(&dummy), line_pitch - width_);
            }
        }
    }

    void resize(size_t width, size_t height) {
        data_.resize(width * height);
        width_  = width;
        height_ = height;
        // depth_ = depth;
    }

    void fill(intensity_t value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    intensity_t &at(size_t x, size_t y, size_t z = 0) {
        assert(x < width_);
        assert(y < height_);
        assert(z < depth_);
        return data_[z * width_ * height_ + y * width_ + x];
    }

    const intensity_t &at(size_t x, size_t y, size_t z = 0) const {
        assert(x < width_);
        assert(y < height_);
        assert(z < depth_);
        return data_[z * width_ * height_ + y * width_ + x];
    }

    intensity_t       &operator[](std::size_t idx)       {return data_[idx]; };
    const intensity_t &operator[](std::size_t idx) const {return data_[idx]; };

    size_t                          width() const  {return width_; }
    size_t                          height() const {return height_; }
    size_t                          depth() const  {return depth_; }
    const std::vector<intensity_t> &data() const   {return data_; }

private:
    size_t                   width_;
    size_t                   height_;
    size_t                   depth_;
    std::vector<intensity_t> data_;
};


/********************************************************************************
 *
 * visualize 1d-vector
 *
 * @example
 *
 * vec:[1,5,3]
 *
 * img:
 *   ----------
 *   -11-55-33-
 *   -11-55-33-
 *   ----------
 *
 ********************************************************************************/
template<typename T = unsigned char>
inline image<T> vec2image(const vec_t &vec, size_t block_size = 2, size_t max_cols = 20) {
    if (vec.empty()) throw std::runtime_error("failed to visialize image: vector is empty");

    image<T>     img;
    const size_t border_width = 1;
    const auto   cols                             = vec.size() >= (size_t)max_cols ? (size_t)max_cols : vec.size();
    const auto   rows                             = (vec.size() - 1) / cols + 1;
    const auto   pitch                            = block_size + border_width;
    const auto   width                            = pitch * cols + border_width;
    const auto   height                           = pitch * rows + border_width;
    const typename image<T>::intensity_t bg_color = 255;
    size_t current_idx = 0;

    img.resize(width, height);
    img.fill(bg_color);

    auto minmax = std::minmax_element(vec.begin(), vec.end());

    for (unsigned int r = 0; r < rows; r++) {
        size_t topy = pitch * r + border_width;

        for (unsigned int c = 0; c < cols; c++, current_idx++) {
            size_t               leftx = pitch * c + border_width;
            const float_t        src   = vec[current_idx];
            image<>::intensity_t dst
                = static_cast<typename image<T>::intensity_t>(rescale(src, *minmax.first, *minmax.second, 0, 255));

            for (size_t y = 0; y < block_size; y++)
                for (size_t x = 0; x < block_size; x++) img.at(x + leftx, y + topy) = dst;


            if (current_idx == vec.size()) return img;
        }
    }
    return img;
}

/********************************************************************************
 *
 * visualize 1d-vector
 *
 * @example
 *
 * vec:[5,2,1,3,6,3,0,9,8,7,4,2] maps:[width=2,height=3,depth=2]
 *
 * img:
 *  -------
 *  -52-09-
 *  -13-87-
 *  -63-42-
 *  -------
 *
 * sample:
 * --------
 *  vec_t v;
 *  v.insert(v.end(), vC.begin(), vC.end());
 *  v.insert(v.end(), vD.begin(), vD.end());
 *  v.insert(v.end(), vE.begin(), vE.end());
 *  auto maps = index3d<size_t>(M, N, 3);
 *  auto im   = vec2image<>(v, maps);
 *  im.write("matrix.bmp");
 * --------
 *
 ********************************************************************************/
template<typename T = unsigned char>
inline image<T> vec2image(const vec_t &vec, const index3d<size_t> &maps) {
    if (vec.empty()) throw std::runtime_error("failed to visualize image: vector is empty");
    if (vec.size() != maps.size()) throw std::runtime_error("failed to visualize image: vector size invalid");

    const size_t border_width = 1;
    const auto   pitch                            = maps.width_ + border_width;
    const auto   width                            = maps.depth_ * pitch + border_width;
    const auto   height                           = maps.height_ + 2 * border_width;
    const typename image<T>::intensity_t bg_color = 255;
    image<T> img;

    img.resize(width, height);
    img.fill(bg_color);

    auto minmax = std::minmax_element(vec.begin(), vec.end());

    for (size_t c = 0; c < maps.depth_; ++c) {
        const auto top  = border_width;
        const auto left = c * pitch + border_width;

        for (size_t y = 0; y < maps.height_; ++y) {
            for (size_t x = 0; x < maps.width_; ++x) {
                const float_t val = vec[maps.get_index(x, y, c)];

                img.at(left + x, top + y)
                    = static_cast<typename image<T>::intensity_t>(rescale(val, *minmax.first, *minmax.second, 0, 255));
            }
        }
    }
    return img;
}
} // namespace cc
