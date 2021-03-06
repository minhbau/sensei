#ifndef DIY_SERIALIZATION_HPP
#define DIY_SERIALIZATION_HPP

#include <vector>
#include <valarray>
#include <map>
#include <set>
#include <string>
#include <fstream>

#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>              // this is used for a safety check for default serialization

namespace sdiy
{
  //! A serialization buffer. \ingroup Serialization
  struct BinaryBuffer
  {
    virtual ~BinaryBuffer()                                         =default;
    virtual void        save_binary(const char* x, size_t count)    =0;   //!< copy `count` bytes from `x` into the buffer
    virtual inline void append_binary(const char* x, size_t count)  =0;   //!< append `count` bytes from `x` to end of buffer
    virtual void        load_binary(char* x, size_t count)          =0;   //!< copy `count` bytes into `x` from the buffer
    virtual void        load_binary_back(char* x, size_t count)     =0;   //!< copy `count` bytes into `x` from the back of the buffer
  };

  struct MemoryBuffer: public BinaryBuffer
  {
                        MemoryBuffer(size_t position_ = 0):
                          position(position_)                       {}

    virtual inline void save_binary(const char* x, size_t count) override;   //!< copy `count` bytes from `x` into the buffer
    virtual inline void append_binary(const char* x, size_t count) override; //!< append `count` bytes from `x` to end of buffer
    virtual inline void load_binary(char* x, size_t count) override;         //!< copy `count` bytes into `x` from the buffer
    virtual inline void load_binary_back(char* x, size_t count) override;    //!< copy `count` bytes into `x` from the back of the buffer

    void                clear()                                     { buffer.clear(); reset(); }
    void                wipe()                                      { std::vector<char>().swap(buffer); reset(); }
    void                reset()                                     { position = 0; }
    void                skip(size_t s)                              { position += s; }
    void                swap(MemoryBuffer& o)                       { std::swap(position, o.position); buffer.swap(o.buffer); }
    bool                empty() const                               { return buffer.empty(); }
    size_t              size() const                                { return buffer.size(); }
    void                reserve(size_t s)                           { buffer.reserve(s); }
                        operator bool() const                       { return position < buffer.size(); }

    //! copy a memory buffer from one buffer to another, bypassing making a temporary copy first
    inline static void  copy(MemoryBuffer& from, MemoryBuffer& to);

    //! multiplier used for the geometric growth of the container
    static float        growth_multiplier()                         { return 1.5; }

    // simple file IO
    void                write(const std::string& fn) const          { std::ofstream out(fn.c_str()); out.write(&buffer[0], size()); }
    void                read(const std::string& fn)
    {
        std::ifstream in(fn.c_str(), std::ios::binary | std::ios::ate);
        buffer.resize(static_cast<size_t>(in.tellg()));
        in.seekg(0);
        in.read(&buffer[0], static_cast<std::streamsize>(size()));
        position = 0;
    }

    size_t              position;
    std::vector<char>   buffer;
  };

  namespace detail
  {
    struct Default {};
  }

  //!\addtogroup Serialization
  //!@{

  /**
   * \brief Main interface to serialization, meant to be specialized for the
   * types that require special handling.  `sdiy::save()` and `sdiy::load()` call
   * the static member functions of this class.
   *
   * The default (unspecialized) version copies
   * `sizeof(T)` bytes from `&x` to or from `bb` via
   * its `sdiy::BinaryBuffer::save_binary()` and `sdiy::BinaryBuffer::load_binary()`
   * functions.  This works out perfectly for plain old data (e.g., simple structs).
   * To save a more complicated type, one has to specialize
   * `sdiy::Serialization<T>` for that type. Specializations are already provided for
   * `std::vector<T>`, `std::map<K,V>`, and `std::pair<T,U>`.
   * As a result one can quickly add a specialization of one's own
   *
   */
  template<class T>
  struct Serialization: public detail::Default
  {
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 5)
    static_assert(std::is_trivially_copyable<T>::value, "Default serialization works only for trivially copyable types");
#endif

    static void         save(BinaryBuffer& bb, const T& x)          { bb.save_binary((const char*)  &x, sizeof(T)); }
    static void         load(BinaryBuffer& bb, T& x)                { bb.load_binary((char*)        &x, sizeof(T)); }
    static size_t       size(const T& x)                            { return sizeof(T); }
  };

  //! Saves `x` to `bb` by calling `sdiy::Serialization<T>::save(bb,x)`.
  template<class T>
  void                  save(BinaryBuffer& bb, const T& x)          { Serialization<T>::save(bb, x); }

  //! Loads `x` from `bb` by calling `sdiy::Serialization<T>::load(bb,x)`.
  template<class T>
  void                  load(BinaryBuffer& bb, T& x)                { Serialization<T>::load(bb, x); }

  //! Optimization for arrays. If `sdiy::Serialization` is not specialized for `T`,
  //! the array will be copied all at once. Otherwise, it's copied element by element.
  template<class T>
  void                  save(BinaryBuffer& bb, const T* x, size_t n);

  //! Optimization for arrays. If `sdiy::Serialization` is not specialized for `T`,
  //! the array will be filled all at once. Otherwise, it's filled element by element.
  template<class T>
  void                  load(BinaryBuffer& bb, T* x, size_t n);

  //! Supports only binary data copying (meant for simple footers).
  template<class T>
  void                  load_back(BinaryBuffer& bb, T& x)           { bb.load_binary_back((char*) &x, sizeof(T)); }

  template<class T, class... Args>
  void                  save(BinaryBuffer& bb, const T& x, const Args&... args)     { save(bb, x); save(bb, args...);}

  template<class T, class... Args>
  void                  load(BinaryBuffer& bb, T& x, Args&... args)                 { load(bb, x); load(bb, args...);}

  //@}


  namespace detail
  {
    template<typename T>
    struct is_default
    {
        typedef char    yes;
        typedef int     no;

        static yes      test(Default*);
        static no       test(...);

        enum { value = (sizeof(test((T*) 0)) == sizeof(yes)) };
    };
  }

  template<class T>
  void                  save(BinaryBuffer& bb, const T* x, size_t n)
  {
    if (!detail::is_default< Serialization<T> >::value)
      for (size_t i = 0; i < n; ++i)
        sdiy::save(bb, x[i]);
    else        // if Serialization is not specialized for U, just save the binary data
      bb.save_binary((const char*) &x[0], sizeof(T)*n);
  }

  template<class T>
  void                  load(BinaryBuffer& bb, T* x, size_t n)
  {
    if (!detail::is_default< Serialization<T> >::value)
      for (size_t i = 0; i < n; ++i)
        sdiy::load(bb, x[i]);
    else      // if Serialization is not specialized for U, just load the binary data
      bb.load_binary((char*) &x[0], sizeof(T)*n);
  }


  // save/load for MemoryBuffer
  template<>
  struct Serialization< MemoryBuffer >
  {
    static void         save(BinaryBuffer& bb, const MemoryBuffer& x)
    {
      sdiy::save(bb, x.position);
      sdiy::save(bb, &x.buffer[0], x.position);
    }

    static void         load(BinaryBuffer& bb, MemoryBuffer& x)
    {
      sdiy::load(bb, x.position);
      x.buffer.resize(x.position);
      sdiy::load(bb, &x.buffer[0], x.position);
    }

    static size_t       size(const MemoryBuffer& x)
    {
        return sizeof(x.position) + x.position;
    }
  };

  // save/load for std::vector<U>
  template<class U>
  struct Serialization< std::vector<U> >
  {
    typedef             std::vector<U>          Vector;

    static void         save(BinaryBuffer& bb, const Vector& v)
    {
      size_t s = v.size();
      sdiy::save(bb, s);
      if (s > 0)
        sdiy::save(bb, &v[0], v.size());
    }

    static void         load(BinaryBuffer& bb, Vector& v)
    {
      size_t s;
      sdiy::load(bb, s);
      v.resize(s);
      if (s > 0)
        sdiy::load(bb, &v[0], s);
    }
  };

  template<class U>
  struct Serialization< std::valarray<U> >
  {
    typedef             std::valarray<U>        ValArray;

    static void         save(BinaryBuffer& bb, const ValArray& v)
    {
      size_t s = v.size();
      sdiy::save(bb, s);
      if (s > 0)
        sdiy::save(bb, &v[0], v.size());
    }

    static void         load(BinaryBuffer& bb, ValArray& v)
    {
      size_t s;
      sdiy::load(bb, s);
      v.resize(s);
      if (s > 0)
        sdiy::load(bb, &v[0], s);
    }
  };

  // save/load for std::string
  template<>
  struct Serialization< std::string >
  {
    typedef             std::string             String;

    static void         save(BinaryBuffer& bb, const String& s)
    {
      size_t sz = s.size();
      sdiy::save(bb, sz);
      sdiy::save(bb, s.c_str(), sz);
    }

    static void         load(BinaryBuffer& bb, String& s)
    {
      size_t sz;
      sdiy::load(bb, sz);
      s.resize(sz);
      for (size_t i = 0; i < sz; ++i)
      {
          char c;
          sdiy::load(bb, c);
          s[i] = c;
      }
    }
  };

  // save/load for std::pair<X,Y>
  template<class X, class Y>
  struct Serialization< std::pair<X,Y> >
  {
    typedef             std::pair<X,Y>          Pair;

    static void         save(BinaryBuffer& bb, const Pair& p)
    {
      sdiy::save(bb, p.first);
      sdiy::save(bb, p.second);
    }

    static void         load(BinaryBuffer& bb, Pair& p)
    {
      sdiy::load(bb, p.first);
      sdiy::load(bb, p.second);
    }
  };

  // save/load for std::map<K,V>
  template<class K, class V>
  struct Serialization< std::map<K,V> >
  {
    typedef             std::map<K,V>           Map;

    static void         save(BinaryBuffer& bb, const Map& m)
    {
      size_t s = m.size();
      sdiy::save(bb, s);
      for (typename std::map<K,V>::const_iterator it = m.begin(); it != m.end(); ++it)
        sdiy::save(bb, *it);
    }

    static void         load(BinaryBuffer& bb, Map& m)
    {
      size_t s;
      sdiy::load(bb, s);
      for (size_t i = 0; i < s; ++i)
      {
        K k;
        sdiy::load(bb, k);
        sdiy::load(bb, m[k]);
      }
    }
  };

  // save/load for std::set<T>
  template<class T>
  struct Serialization< std::set<T> >
  {
    typedef             std::set<T>             Set;

    static void         save(BinaryBuffer& bb, const Set& m)
    {
      size_t s = m.size();
      sdiy::save(bb, s);
      for (typename std::set<T>::const_iterator it = m.begin(); it != m.end(); ++it)
        sdiy::save(bb, *it);
    }

    static void         load(BinaryBuffer& bb, Set& m)
    {
      size_t s;
      sdiy::load(bb, s);
      for (size_t i = 0; i < s; ++i)
      {
        T p;
        sdiy::load(bb, p);
        m.insert(p);
      }
    }
  };

  // save/load for std::unordered_map<K,V,H,E,A>
  template<class K, class V, class H, class E, class A>
  struct Serialization< std::unordered_map<K,V,H,E,A> >
  {
    typedef             std::unordered_map<K,V,H,E,A>   Map;

    static void         save(BinaryBuffer& bb, const Map& m)
    {
      size_t s = m.size();
      sdiy::save(bb, s);
      for (auto& x : m)
        sdiy::save(bb, x);
    }

    static void         load(BinaryBuffer& bb, Map& m)
    {
      size_t s;
      sdiy::load(bb, s);
      for (size_t i = 0; i < s; ++i)
      {
        std::pair<K,V> p;
        sdiy::load(bb, p);
        m.emplace(std::move(p));
      }
    }
  };

  // save/load for std::unordered_set<T,H,E,A>
  template<class T, class H, class E, class A>
  struct Serialization< std::unordered_set<T,H,E,A> >
  {
    typedef             std::unordered_set<T,H,E,A>     Set;

    static void         save(BinaryBuffer& bb, const Set& m)
    {
      size_t s = m.size();
      sdiy::save(bb, s);
      for (auto& x : m)
        sdiy::save(bb, x);
    }

    static void         load(BinaryBuffer& bb, Set& m)
    {
      size_t s;
      sdiy::load(bb, s);
      for (size_t i = 0; i < s; ++i)
      {
        T p;
        sdiy::load(bb, p);
        m.emplace(std::move(p));
      }
    }
  };

  // save/load for std::tuple<...>
  // TODO: this ought to be default (copying) serialization
  //       if all arguments are default
  template<class... Args>
  struct Serialization< std::tuple<Args...> >
  {
    typedef             std::tuple<Args...>     Tuple;

    static void         save(BinaryBuffer& bb, const Tuple& t)          { save<0>(bb, t); }

    template<std::size_t I = 0>
    static
    typename std::enable_if<I == sizeof...(Args), void>::type
                        save(BinaryBuffer&, const Tuple&)               {}

    template<std::size_t I = 0>
    static
    typename std::enable_if<I < sizeof...(Args), void>::type
                        save(BinaryBuffer& bb, const Tuple& t)          { sdiy::save(bb, std::get<I>(t)); save<I+1>(bb, t); }

    static void         load(BinaryBuffer& bb, Tuple& t)                { load<0>(bb, t); }

    template<std::size_t I = 0>
    static
    typename std::enable_if<I == sizeof...(Args), void>::type
                        load(BinaryBuffer&, Tuple&)                     {}

    template<std::size_t I = 0>
    static
    typename std::enable_if<I < sizeof...(Args), void>::type
                        load(BinaryBuffer& bb, Tuple& t)                { sdiy::load(bb, std::get<I>(t)); load<I+1>(bb, t); }

  };
}

void
sdiy::MemoryBuffer::
save_binary(const char* x, size_t count)
{
  if (position + count > buffer.capacity())
  {
    double newsize = static_cast<double>(position + count) * growth_multiplier();  // if we have to grow, grow geometrically
    buffer.reserve(static_cast<size_t>(newsize));
  }

  if (position + count > buffer.size())
    buffer.resize(position + count);

  std::copy_n(x, count, &buffer[position]);
  position += count;
}

void
sdiy::MemoryBuffer::
append_binary(const char* x, size_t count)
{
    size_t temp_pos = position;
    position = size();
    save_binary(x, count);
    position = temp_pos;
}

void
sdiy::MemoryBuffer::
load_binary(char* x, size_t count)
{
  std::copy_n(&buffer[position], count, x);
  position += count;
}

void
sdiy::MemoryBuffer::
load_binary_back(char* x, size_t count)
{
  std::copy_n(&buffer[buffer.size() - count], count, x);
  buffer.resize(buffer.size() - count);
}

void
sdiy::MemoryBuffer::
copy(MemoryBuffer& from, MemoryBuffer& to)
{
  size_t sz;
  sdiy::load(from, sz);
  from.position -= sizeof(size_t);

  size_t total = sizeof(size_t) + sz;
  to.buffer.resize(to.position + total);
  std::copy_n(&from.buffer[from.position], total, &to.buffer[to.position]);
  to.position += total;
  from.position += total;
}

#endif
