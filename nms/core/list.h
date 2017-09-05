#pragma once

#include <nms/core/view.h>
#include <nms/core/memory.h>
#include <nms/core/exception.h>

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
    using Tinfo = typename base::Tinfo;

#pragma region constructor
    constexpr List() noexcept
        : base{nullptr, 0}
    {}

    ~List() {
        if (data_ == nullptr) {
            return;
        }

        for(Tsize i = 0; i < size_; ++i) {
            data_[i].~Tdata();
        }

        const auto buff_ = buff();
        if (data_ != buff_) {
            mdel(data_);
        }

        size_ = 0;
        data_ = nullptr;
    }

    template<class ...U>
    explicit List(Tsize count, U&& ...us) {
        appends(count, fwd<U>(us)...);
    }

    List(List&& rhs) noexcept
        : base(rhs)
    {
        rhs.data_     = nullptr;
        rhs.size_     = 0;
    }

    List(const List& rhs) noexcept
        : List{}
    {
        appends(rhs.data(), rhs.count());
    }

    List& operator=(List&& rhs) noexcept {
        base::operator=(rhs);
        rhs.data_ = nullptr;
        rhs.size_ = 0;
        return *this;
    }

    List& operator=(const List& rhs) noexcept {
        // clear
        for (Tsize i = 0; i < size_; ++i) {
            data_[i].~Tdata();
        }
        size_ = 0;

        // append
        appends(rhs.data(), rhs.count());
        return *this;
    }
#pragma endregion

#pragma region property
    using base::data;
    using base::size;
    using base::count;

    /* the number of elements that can be held in currently allocated storage */
    Tsize capacity() const noexcept {
        return capacity_;
    }
#pragma endregion

#pragma region method
    List& reserve(Tsize newcnt) {
        const auto oldcnt = base::size_;
        const auto oldcap = capacity_;
        if (newcnt > oldcap) {
            // caculate new capicity
            newcnt = newcnt > oldcap + oldcap/16 ? newcnt : oldcap + oldcap/16;
            const auto newcap = (newcnt +31) / 32 * 32;
            const auto olddat = data_;
            capacity_ = newcap;

            // realloc
            const auto newdat = mnew<Tdata>(newcap);
            data_ = newdat;

            // move olddat -> newdat
            for(Tsize i = 0; i < oldcnt; ++i) {
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
#pragma endregion

#pragma region save/load
    void save(io::File& file) const {
        saveFile(*this, file);
    }

    static List load(const io::File& file) {
        List list;
        loadFile(list, file);
        return list;
    }
#pragma endregion

protected:
    using   base::data_;
    using   base::size_;
    using   base::capacity_;

    Tdata* buff() {
        return reinterpret_cast<Tdata*>(&capacity_ + 1);
    }

    const Tdata* buff() const {
        return reinterpret_cast<const Tdata*>(&capacity_+1);
    }

    template<class File>
    static void saveFile(const List& list, File& file) {
        const auto info = list.info();
        const auto dims = list.size();
        const auto data = list.data();
        const auto nums = list.count();
        file.write(&info, 1);
        file.write(&dims, 1);
        file.write(data, nums);
    }

    template<class File>
    static void loadFile(List& list, const File& file) {
        Tinfo info;
        file.read(&info, 1);
        if (info != base::info()) { 
            NMS_THROW(EBadType{});
        }

        typename base::Tdims dims;
        file.read(&dims, 1);

        list.reserve(dims[0]);
        file.read(list.data(), dims[0]);
    }
};

template<class T, u32 N>
class List: public List<T, 0>
{
public:
    using base = List<T, 0>;
    using Tdata = typename base::Tdata;
    using Tsize = typename base::Tsize;

    static const Tsize $capicity = N;

#pragma region constructor
    constexpr List() noexcept {
        data_       = reinterpret_cast<T*>(buff_);
        capacity_   = $capicity;
    }

    ~List()
    {}

    template<class ...U>
    explicit List(Tsize count, U&& ...us)
        : List{}
    {
        appends(count, fwd<U>(us)...);
    }

    List(List&& rhs) noexcept
        : base{ static_cast<base&&>(rhs) }
    {
        if (data() == rhs.buff()) {
            dat2buf();
        }
    }

    List& operator=(List&& rhs) noexcept {
        if (this != &rhs) {
            base::operator=(static_cast<base&&>(rhs));
            if (rhs.data() == rhs.buff()) {
                dat2buf();
            }
        }
        return *this;
    }
#pragma endregion

#pragma region property
    using base::data;
    using base::size;
    using base::count;
    using base::buff;
#pragma endregion

#pragma region save/load
    static List load(const io::File& file) {
        List list;
        base::loadFile(list, file);
        return list;
    }
#pragma endregion

protected:
    using   base::data_;
    using   base::size_;
    using   base::capacity_;
    u8      buff_[sizeof(Tdata)*$capicity] = {};

    void dat2buf() {
        auto buf = buff();
        auto dat = data();
        for (Tsize i = 0; i < size_; ++i) {
            new(&buf[i]) Tdata(static_cast<Tdata&&>(dat[i]));
        }
        data_ = buf;
    }

};

}

