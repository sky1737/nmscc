#pragma once

#include <nms/core/view.h>
#include <nms/core/memory.h>

namespace nms
{

namespace io
{
class File;
class Path;
}

template<class T, u32 S = 0>
class List;

/* list */
template<class T>
class List<T, 0>: public View<T>
{
    
public:
    using base  = View<T>;
    using Tdata = typename base::Tdata;
    using Tsize = typename base::Tsize;

#pragma region constructor
    constexpr List() noexcept = default;

    ~List() {
        if (base::data_ == nullptr) {
            return;
        }

        for(Tsize i = 0; i < size_; ++i) {
            base::data_[i].~Tdata();
        }

        if (data_ != buff()) {
            mdel(base::data_);
        }
    }
#pragma endregion

#pragma region property
    using base::data;
    using base::size;
    using base::count;

    /* the number of elements that can be held in currently allocated storage */
    Tsize capicity() const noexcept {
        return capicity_;
    }
#pragma endregion

#pragma region method
    List& reserve(Tsize newcnt) {
        const auto oldcnt = base::size_;
        const auto oldcap = capicity_;
        if (newcnt > oldcap) {
            // caculate new capicity
            newcnt = newcnt > oldcap + oldcap/16 ? newcnt : oldcap + oldcap/16;
            const auto newcap = (newcnt +31) / 32 * 32;
            const auto olddat = data_;

            // realloc
            const auto newdat = mnew<Tdata>(newcap);
            data_ = newdat;

            // move olddat -> newdat
            for(auto i = 0; i < oldcnt; ++i) {
                new (&newdat[i])Tdata(static_cast<Tdata&&>(olddat[i]));
            }

            // free old data
            if (olddat!=nullptr && olddat!=buff()) {
                mdel(olddat);
            }
            
        }
        return *this;
    }

    /*! append elements to the end */
    template<class ...U>
    List& append(U&& ...u) {
        reserve(size_ + 1);
        new(&data_[size_++]) Tdata(fwd<U>(u)...);
        return *this;
    }

    /*! append elements to the end */
    template<class ...U>
    List& appends(Tsize cnt, U&& ...u) {
        reserve(size_ + cnt);
        for (Tsize i = 0; i < cnt; ++i) {
            new(&data_[base::size_++])Tdata(fwd<U>(u)...);
        }
        return *this;
    }

    /*! append(copy) elements to the end */
    template<class U>
    List& appends(const U dat[], Tsize cnt) {
        reserve(size_ + cnt);
        for (Tsize i = 0; i < cnt; ++i) {
            new(&data_[base::size_++])Tdata(dat[i]);
        }

        return *this;
    }

    /*! append an element to the end */
    template<class U>
    List& operator+=(U&& u) {
        reserve(size_ + 1);
        new(&data_[size_++]) Tdata(fwd<U>(u));
        return *this;
    }

protected:
    using   base::data_;
    using   base::size_;
    Tsize   capicity_;

    const Tdata* buff() const {
        return static_cast<const void*>(&capicity+1);
    }

#pragma region save/load
    void save(io::File& file)       const;
    void save(const io::Path& path) const;

    static List load(io::File& file);
    static List load(const io::Path& path);
#pragma endregion
};

template<class T, u32 S>
class List: public List<T, 0>
{
    using base = List<T>;

public:
    static const auto $caicity = S;

    using Tdata = typename base::Tdata;
    using Tsize = typename base::Tsize;
    
#pragma region constructor
    constexpr List() noexcept
    {
        data_       = static_cast<T*>(buff_);
        capicity_   = $capicity;
    }

    ~List() = default;
#pragma endregion

#pragma region property
    using base::data;
    using base::size;
    using base::count;

#pragma endregion

protected:
    using   base::data_;
    using   base::size_;
    using   base::capicity_;
    u8      buff_[sizeof(Tdata)*$capicity];
    
    const Tdata* buff() const {
        return static_cast<const void*>(&capicity+1);
    }

#pragma region save/load
    static List load(io::File& file);
    static List load(const io::Path& path);
#pragma endregion
};


}

